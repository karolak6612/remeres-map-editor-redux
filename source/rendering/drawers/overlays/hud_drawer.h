#ifndef RME_RENDERING_DRAWERS_OVERLAYS_HUD_DRAWER_H_
#define RME_RENDERING_DRAWERS_OVERLAYS_HUD_DRAWER_H_

#include <memory>
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"

struct NVGcontext;
class Editor;

class HUDDrawer {
public:
	HUDDrawer();
	~HUDDrawer();

	void draw(NVGcontext* vg, const RenderView& view, Editor& editor, const DrawingOptions& options);

private:
	void DrawCoords(NVGcontext* vg, const RenderView& view, Editor& editor);
	void DrawBrushInfo(NVGcontext* vg, const RenderView& view, Editor& editor);
	void DrawSelectionInfo(NVGcontext* vg, const RenderView& view, Editor& editor, const DrawingOptions& options);
};

#endif
