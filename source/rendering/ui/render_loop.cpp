#include "app/main.h"

#include "rendering/ui/render_loop.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <sstream>

#include "rendering/core/frame_builder.h"
#include "rendering/core/graphics.h"
#include "rendering/core/text_renderer.h"
#include "rendering/core/tile_planning_pool.h"
#include "rendering/map_drawer.h"
#include "rendering/ui/gl_context_manager.h"

#include <glad/glad.h>
#include <nanovg.h>
#include <spdlog/spdlog.h>
#include <wx/gdicmn.h>
#include <wx/glcanvas.h>

namespace rme::rendering {

namespace {

[[nodiscard]] size_t CountPreparedCommands(const PreparedFrameBuffer& prepared)
{
	size_t total = 0;
	for (const auto& floor : prepared.floors) {
		for (const auto& chunk : floor.chunks) {
			if (chunk) {
				total += chunk->commands.size();
			}
		}
	}
	return total;
}

} // namespace

RenderLoop::RenderLoop(MapDrawer& drawer, GLContextManager& gl, Editor& editor, RenderLoopHost& host) :
	drawer_(drawer),
	gl_(gl),
	editor_(editor),
	host_(host) {
	planning_pool_ = std::make_unique<TilePlanningPool>(std::max<size_t>(1, host_.planningWorkerCount()));
	drawer_.SetPlanningPool(planning_pool_.get());
	spdlog::info(
		"RenderLoop: initialized threaded prep infrastructure (planning worker threads: {}, caller participates: yes)",
		planning_pool_->workerCount()
	);
	if (IsThreadedPipelineReady()) {
		prep_thread_.emplace([this](std::stop_token stop_token) { PrepThreadMain(stop_token); });
		spdlog::info("RenderLoop: prep coordinator thread started");
	}
}

RenderLoop::~RenderLoop() = default;

bool RenderLoop::IsThreadedPipelineReady() {
	return true;
}

void RenderLoop::ExecuteFrame() {
	static_cast<void>(editor_);
	PollThreadedFailure();

	const bool has_context = gl_.MakeCurrent();
	NVGcontext* vg = has_context ? gl_.GetNVG() : nullptr;

	if (has_context && host_.isRenderingEnabled()) {
		host_.graphics().updateTime();

		const auto settings = host_.buildRenderSettings();
		const auto frame_options = host_.buildFrameOptions();
		const auto brush_visual = host_.buildBrushVisualSettings();
		host_.updateAnimationState(settings.show_preview);

		const auto snapshot = host_.buildViewSnapshot();
		const auto brush = host_.buildBrushSnapshot();
		const bool threaded_rendering_requested = host_.isThreadedRenderingEnabled();
		const bool threaded_rendering = threaded_rendering_requested && IsThreadedPipelineReady()
			&& !threaded_rendering_session_disabled_.load(std::memory_order_acquire);
		if (threaded_rendering_requested && !threaded_rendering) {
			if (!IsThreadedPipelineReady()) {
				std::call_once(threaded_rendering_warning_once_, [] {
					spdlog::warn("Threaded rendering is disabled because the snapshot-driven pipeline is not ready yet.");
				});
			} else if (threaded_rendering_session_disabled_.load(std::memory_order_acquire)) {
				std::call_once(threaded_rendering_disabled_warning_once_, [] {
					spdlog::warn("Threaded rendering was disabled for this session after a validation or prep-thread failure.");
				});
			}
		}

		if (threaded_rendering) {
			std::call_once(threaded_rendering_active_once_, [] {
				spdlog::info("RenderLoop: threaded rendering is active for this session");
			});
			auto prep_snapshot = drawer_.BuildRenderPrepSnapshot(snapshot, brush, brush_visual, settings, frame_options);
			prep_snapshot.generation = ++next_generation_;
			if (validation_.shouldSample(prep_snapshot.generation)) {
				validation_.trackSnapshot(prep_snapshot);
			}
			EnqueuePreparedFrame(std::move(prep_snapshot));
			if (!drawer_.HasPreparedFrame()) {
				const bool prepared_ready = WaitForPreparedFrame(next_generation_);
				PollThreadedFailure();
				if (!threaded_rendering_session_disabled_.load(std::memory_order_acquire) && prepared_ready) {
					(void)TryApplyPreparedFrame();
				}
				if (!drawer_.HasPreparedFrame()) {
					DisableThreadedRenderingForSession("Could not produce an initial prepared frame.");
				}
			} else if (!threaded_rendering_session_disabled_.load(std::memory_order_acquire)) {
				(void)TryApplyPreparedFrame();
			}
		}
		if (!threaded_rendering || threaded_rendering_session_disabled_.load(std::memory_order_acquire)) {
			drawer_.SetupVars(snapshot, brush, brush_visual, settings, frame_options);
		} else {
			(void)TryApplyPreparedFrame();
		}
		drawer_.SetupGL();
		drawer_.Draw();
		drawer_.DrainPendingNodeRequests();

		if (host_.isCapturingScreenshot()) {
			drawer_.TakeScreenshot(host_.screenshotBuffer());
		}

		drawer_.Release();
		DrawOverlays(vg);
		drawer_.BeginFrame();
	}

	PerformGarbageCollection();

	if (has_context) {
		host_.canvas().SwapBuffers();
	}

	host_.updateFramePacing();
	host_.sendNodeRequests();
}

void RenderLoop::DrawOverlays(NVGcontext* vg) const {
	if (!vg) {
		return;
	}

	const auto& settings = drawer_.getRenderSettings();

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glClear(GL_STENCIL_BUFFER_BIT);
	const wxSize size = host_.canvas().GetSize();
	TextRenderer::BeginFrame(vg, size.x, size.y, host_.canvas().GetContentScaleFactor());

	if (settings.show_creatures) {
		drawer_.DrawCreatureNames(vg);
	}
	if (settings.show_tooltips) {
		drawer_.DrawTooltips(vg);
	}
	if (settings.show_hooks) {
		drawer_.DrawHookIndicators(vg);
	}
	if (settings.highlight_locked_doors) {
		drawer_.DrawDoorIndicators(vg);
	}

	TextRenderer::EndFrame(vg);
	glUseProgram(0);
	glBindVertexArray(0);
}

void RenderLoop::PerformGarbageCollection() const {
	if (host_.shouldCollectGarbage()) {
		host_.collectGarbage();
	}
}

void RenderLoop::PrepThreadMain(std::stop_token stop_token) {
	while (!stop_token.stop_requested()) {
		std::optional<RenderPrepSnapshot> snapshot;
		{
			std::unique_lock lock(prep_mutex_);
			prep_cv_.wait(lock, stop_token, [&] { return pending_snapshot_.has_value(); });
			if (stop_token.stop_requested()) {
				return;
			}
			snapshot = std::move(pending_snapshot_);
			pending_snapshot_.reset();
		}

		if (!snapshot) {
			continue;
		}

		PreparedFrameBuffer prepared;
		try {
			prepared = drawer_.PrepareFrame(std::move(*snapshot));
		} catch (const std::exception& exception) {
			std::lock_guard lock(prep_mutex_);
			pending_failure_reason_ = std::string("Prep thread failed: ") + exception.what();
			prep_cv_.notify_all();
			return;
		} catch (...) {
			std::lock_guard lock(prep_mutex_);
			pending_failure_reason_ = "Prep thread failed with an unknown exception.";
			prep_cv_.notify_all();
			return;
		}

		std::lock_guard lock(prep_mutex_);
		if (threaded_rendering_session_disabled_.load(std::memory_order_acquire)) {
			continue;
		}
		if (!prepared_frame_ || prepared.generation >= prepared_frame_->generation) {
			spdlog::debug(
				"RenderLoop: prepared frame published (generation={}, floors={}, chunks={}, commands={}, lights={})",
				prepared.generation,
				prepared.floors.size(),
				std::accumulate(
					prepared.floors.begin(), prepared.floors.end(), static_cast<size_t>(0),
					[](size_t total, const PreparedVisibleFloor& floor) { return total + floor.chunks.size(); }
				),
				CountPreparedCommands(prepared),
				prepared.lights.size()
			);
			prepared_frame_ = std::move(prepared);
		}
		prep_cv_.notify_all();
	}
}

void RenderLoop::EnqueuePreparedFrame(RenderPrepSnapshot snapshot) {
	std::lock_guard lock(prep_mutex_);
	pending_snapshot_ = std::move(snapshot);
	prep_cv_.notify_one();
}

bool RenderLoop::TryApplyPreparedFrame() {
	std::optional<PreparedFrameBuffer> prepared;
	{
		std::lock_guard lock(prep_mutex_);
		if (!prepared_frame_ || prepared_frame_->generation <= last_applied_generation_) {
			return false;
		}
		prepared = std::move(prepared_frame_);
		prepared_frame_.reset();
	}

	if (auto validation_result = validation_.validatePreparedFrame(drawer_, *prepared)) {
		if (!validation_result->ok) {
			std::ostringstream stream;
			stream << "Prepared frame validation failed for generation " << validation_result->generation << ": "
				<< validation_result->message;
			DisableThreadedRenderingForSession(stream.str());
			return false;
		}
		spdlog::info("RenderLoop: validation passed for generation {}", validation_result->generation);
	}

	const bool is_initial_prepared_frame = applied_prepared_frames_ == 0;
	const auto prepared_generation = prepared->generation;
	const auto prepared_command_count = CountPreparedCommands(*prepared);
	const auto prepared_light_count = prepared->lights.size();
	const auto prepared_floor_count = prepared->floors.size();
	drawer_.SetupPreparedFrame(std::move(*prepared));
	last_applied_generation_ = prepared->generation;
	applied_prepared_frames_++;
	if (is_initial_prepared_frame) {
		std::call_once(initial_prepared_frame_once_, [&] {
			spdlog::info(
				"RenderLoop: initial prepared frame applied (generation={}, total commands={}, lights={}, floors={})",
				prepared_generation,
				prepared_command_count,
				prepared_light_count,
				prepared_floor_count
			);
		});
	}
	validation_.discardOlderThan(last_applied_generation_);
	return true;
}

bool RenderLoop::WaitForPreparedFrame(uint64_t minimum_generation) {
	std::unique_lock lock(prep_mutex_);
	return prep_cv_.wait_for(lock, std::chrono::milliseconds(250), [&] {
		return pending_failure_reason_.has_value()
			|| (prepared_frame_.has_value() && prepared_frame_->generation >= minimum_generation);
	});
}

void RenderLoop::DisableThreadedRenderingForSession(std::string reason) {
	const bool already_disabled = threaded_rendering_session_disabled_.exchange(true, std::memory_order_acq_rel);
	if (!already_disabled) {
		spdlog::error("Threaded rendering disabled for this session: {}", reason);
	}

	std::lock_guard lock(prep_mutex_);
	pending_snapshot_.reset();
	prepared_frame_.reset();
	validation_.clear();
	prep_cv_.notify_all();
}

void RenderLoop::PollThreadedFailure() {
	std::optional<std::string> failure_reason;
	{
		std::lock_guard lock(prep_mutex_);
		if (!pending_failure_reason_.has_value()) {
			return;
		}
		failure_reason = std::move(pending_failure_reason_);
		pending_failure_reason_.reset();
	}

	if (failure_reason) {
		DisableThreadedRenderingForSession(*failure_reason);
	}
}

} // namespace rme::rendering
