#ifndef RME_RENDERING_UI_RENDER_LOOP_H_
#define RME_RENDERING_UI_RENDER_LOOP_H_

#include "rendering/core/brush_snapshot.h"
#include "rendering/core/brush_visual_settings.h"
#include "rendering/core/draw_frame.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/prepared_frame_buffer.h"
#include "rendering/core/render_prep_snapshot.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_validation_layer.h"
#include "rendering/core/view_snapshot.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

class wxGLCanvas;
struct NVGcontext;
class MapDrawer;
class Editor;
class GraphicManager;
class TilePlanningPool;

namespace rme::rendering {

class GLContextManager;

class RenderLoopHost {
public:
	virtual ~RenderLoopHost() = default;

	[[nodiscard]] virtual wxGLCanvas& canvas() = 0;
	[[nodiscard]] virtual bool isRenderingEnabled() const = 0;
	[[nodiscard]] virtual bool isThreadedRenderingEnabled() const = 0;
	[[nodiscard]] virtual size_t planningWorkerCount() const = 0;
	[[nodiscard]] virtual GraphicManager& graphics() const = 0;
	[[nodiscard]] virtual RenderSettings buildRenderSettings() const = 0;
	[[nodiscard]] virtual FrameOptions buildFrameOptions() const = 0;
	[[nodiscard]] virtual ViewSnapshot buildViewSnapshot() const = 0;
	[[nodiscard]] virtual BrushSnapshot buildBrushSnapshot() const = 0;
	[[nodiscard]] virtual BrushVisualSettings buildBrushVisualSettings() const = 0;
	virtual void updateAnimationState(bool show_preview) = 0;
	[[nodiscard]] virtual bool isCapturingScreenshot() const = 0;
	[[nodiscard]] virtual uint8_t* screenshotBuffer() const = 0;
	[[nodiscard]] virtual bool shouldCollectGarbage() const = 0;
	virtual void collectGarbage() = 0;
	virtual void updateFramePacing() = 0;
	virtual void sendNodeRequests() = 0;
};

class RenderLoop {
public:
	RenderLoop(MapDrawer& drawer, GLContextManager& gl, Editor& editor, RenderLoopHost& host);
	~RenderLoop();
	[[nodiscard]] static bool IsThreadedPipelineReady();

	void ExecuteFrame();

private:
	void DrawOverlays(NVGcontext* vg) const;
	void PerformGarbageCollection() const;
	void PrepThreadMain(std::stop_token stop_token);
	void EnqueuePreparedFrame(RenderPrepSnapshot snapshot);
	bool TryApplyPreparedFrame();
	bool WaitForPreparedFrame(uint64_t minimum_generation);
	void DisableThreadedRenderingForSession(std::string reason);
	void PollThreadedFailure();

	MapDrawer& drawer_;
	GLContextManager& gl_;
	Editor& editor_;
	RenderLoopHost& host_;
	std::unique_ptr<TilePlanningPool> planning_pool_;
	std::optional<std::jthread> prep_thread_;
	mutable std::mutex prep_mutex_;
	std::condition_variable_any prep_cv_;
	std::optional<RenderPrepSnapshot> pending_snapshot_;
	std::optional<PreparedFrameBuffer> prepared_frame_;
	std::optional<std::string> pending_failure_reason_;
	uint64_t next_generation_ = 0;
	uint64_t last_applied_generation_ = 0;
	size_t applied_prepared_frames_ = 0;
	std::atomic<bool> threaded_rendering_session_disabled_ {false};
	RenderValidationLayer validation_;
	std::once_flag threaded_rendering_warning_once_;
	std::once_flag threaded_rendering_disabled_warning_once_;
	std::once_flag threaded_rendering_active_once_;
	std::once_flag initial_prepared_frame_once_;
};

} // namespace rme::rendering

#endif
