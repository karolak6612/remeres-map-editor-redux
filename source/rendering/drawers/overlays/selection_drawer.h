#ifndef RME_RENDERING_SELECTION_DRAWER_H_
#define RME_RENDERING_SELECTION_DRAWER_H_

#include "rendering/core/draw_context.h"

class MapCanvas;

class SelectionDrawer {
public:
	void draw(const DrawContext& ctx, const MapCanvas* canvas);
};

#endif
