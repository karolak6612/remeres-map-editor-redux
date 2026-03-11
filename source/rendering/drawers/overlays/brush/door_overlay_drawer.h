#ifndef RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_DOOR_OVERLAY_DRAWER_H_
#define RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_DOOR_OVERLAY_DRAWER_H_

struct DrawContext;
struct BrushOverlayContext;

class DoorOverlayDrawer {
public:
	void draw(const DrawContext& ctx, const BrushOverlayContext& overlay);
};

#endif
