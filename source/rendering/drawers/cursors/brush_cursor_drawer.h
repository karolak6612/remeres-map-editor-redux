#ifndef RME_RENDERING_BRUSH_CURSOR_DRAWER_H_
#define RME_RENDERING_BRUSH_CURSOR_DRAWER_H_

#include <cstdint>

#include "rendering/core/draw_context.h"

class Brush;

class BrushCursorDrawer {
public:
	void draw(const DrawContext& ctx, int x, int y, Brush* brush, uint8_t r, uint8_t g, uint8_t b);
};

#endif
