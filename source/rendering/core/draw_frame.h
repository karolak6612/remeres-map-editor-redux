#ifndef RME_RENDERING_CORE_DRAW_FRAME_H_
#define RME_RENDERING_CORE_DRAW_FRAME_H_

#include "rendering/core/brush_snapshot.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/view_snapshot.h"

class AtlasManager;

// Bundles all per-frame mutable state into a single value type.
// Isolates frame-scoped data from long-lived MapDrawer members,
// preparing for future multi-threaded frame construction.
struct DrawFrame {
	RenderSettings settings;
	FrameOptions options;
	ViewState view;
	ViewSnapshot snapshot;
	BrushSnapshot brush;
	LightBuffer lights;
	AtlasManager* atlas = nullptr;
};

#endif
