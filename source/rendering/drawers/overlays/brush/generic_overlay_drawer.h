#ifndef RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_GENERIC_OVERLAY_DRAWER_H_
#define RME_RENDERING_DRAWERS_OVERLAYS_BRUSH_GENERIC_OVERLAY_DRAWER_H_

#include <glm/glm.hpp>

struct DrawContext;
struct BrushOverlayContext;

class GenericOverlayDrawer {
public:
	void draw(const DrawContext& ctx, const BrushOverlayContext& overlay, const glm::vec4& brush_color);
};

#endif
