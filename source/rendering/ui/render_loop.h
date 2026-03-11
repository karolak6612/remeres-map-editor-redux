#ifndef RME_RENDERING_UI_RENDER_LOOP_H_
#define RME_RENDERING_UI_RENDER_LOOP_H_

#include "rendering/core/brush_snapshot.h"
#include "rendering/core/brush_visual_settings.h"
#include "rendering/core/draw_frame.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/prepared_frame_buffer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/view_snapshot.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

class wxGLCanvas;
struct NVGcontext;
class AtlasManager;
class MapDrawer;
class Editor;
class GraphicManager;

namespace rme::rendering {

class GLContextManager;

struct RenderPrepSnapshot {
	uint64_t generation = 0;
	ViewSnapshot view;
	BrushSnapshot brush;
	BrushVisualSettings brush_visual;
	RenderSettings settings;
	FrameOptions frame_options;
	AtlasManager* atlas = nullptr;
};

class RenderLoopHost {
public:
	virtual ~RenderLoopHost() = default;

	[[nodiscard]] virtual wxGLCanvas& canvas() = 0;
	[[nodiscard]] virtual bool isRenderingEnabled() const = 0;
	[[nodiscard]] virtual bool isThreadedRenderingEnabled() const = 0;
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
	[[nodiscard]] static bool IsThreadedPipelineReady();

	void ExecuteFrame();

private:
	void DrawOverlays(NVGcontext* vg) const;
	void PerformGarbageCollection() const;
	void PrepThreadMain(std::stop_token stop_token);
	void EnqueuePreparedFrame(RenderPrepSnapshot snapshot);
	bool TryApplyPreparedFrame();

	MapDrawer& drawer_;
	GLContextManager& gl_;
	Editor& editor_;
	RenderLoopHost& host_;
	std::optional<std::jthread> prep_thread_;
	mutable std::mutex prep_mutex_;
	std::condition_variable_any prep_cv_;
	std::optional<RenderPrepSnapshot> pending_snapshot_;
	std::optional<PreparedFrameBuffer> prepared_frame_;
	uint64_t next_generation_ = 0;
	uint64_t last_applied_generation_ = 0;
	std::once_flag threaded_rendering_warning_once_;
};

} // namespace rme::rendering

#endif
