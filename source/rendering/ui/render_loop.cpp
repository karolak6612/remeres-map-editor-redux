#include "app/main.h"

#include "rendering/ui/render_loop.h"

#include "rendering/core/frame_builder.h"
#include "rendering/core/graphics.h"
#include "rendering/core/text_renderer.h"
#include "rendering/map_drawer.h"
#include "rendering/ui/gl_context_manager.h"

#include <glad/glad.h>
#include <nanovg.h>
#include <spdlog/spdlog.h>
#include <wx/gdicmn.h>
#include <wx/glcanvas.h>

namespace rme::rendering {

RenderLoop::RenderLoop(MapDrawer& drawer, GLContextManager& gl, Editor& editor, RenderLoopHost& host) :
	drawer_(drawer),
	gl_(gl),
	editor_(editor),
	host_(host) {
	if (IsThreadedPipelineReady()) {
		prep_thread_.emplace([this](std::stop_token stop_token) { PrepThreadMain(stop_token); });
	}
}

bool RenderLoop::IsThreadedPipelineReady() {
	// The user-facing setting stays visible, but the old DrawFrame-only prep
	// path is intentionally disabled until the full snapshot-driven pipeline
	// can prepare map traversal, command buffers, and accumulators safely.
	return false;
}

void RenderLoop::ExecuteFrame() {
	static_cast<void>(editor_);

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
		const bool threaded_rendering = threaded_rendering_requested && IsThreadedPipelineReady();
		if (threaded_rendering_requested && !threaded_rendering) {
			std::call_once(threaded_rendering_warning_once_, [] {
				spdlog::warn("Threaded rendering is disabled because the snapshot-driven pipeline is not ready yet.");
			});
		}
		bool used_prepared_frame = false;
		if (threaded_rendering) {
			used_prepared_frame = TryApplyPreparedFrame();
			EnqueuePreparedFrame(RenderPrepSnapshot {
				.generation = ++next_generation_,
				.view = snapshot,
				.brush = brush,
				.brush_visual = brush_visual,
				.settings = settings,
				.frame_options = frame_options,
				.atlas = host_.graphics().getAtlasManager(),
			});
		}
		if (!used_prepared_frame) {
			drawer_.SetupVars(snapshot, brush, brush_visual, settings, frame_options);
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

		PreparedFrameBuffer prepared {
			.generation = snapshot->generation,
			.frame = FrameBuilder::Build(
				snapshot->view, snapshot->brush, snapshot->brush_visual, snapshot->settings, snapshot->frame_options, editor_, snapshot->atlas
			),
		};
		prepared.reserve(512, 256, 128, 64, 4096);

		std::lock_guard lock(prep_mutex_);
		if (!prepared_frame_ || prepared.generation >= prepared_frame_->generation) {
			prepared_frame_ = std::move(prepared);
		}
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

	drawer_.SetupVars(std::move(prepared->frame));
	last_applied_generation_ = prepared->generation;
	return true;
}

} // namespace rme::rendering
