#ifndef RME_RENDERING_LIVE_CURSOR_DRAWER_H_
#define RME_RENDERING_LIVE_CURSOR_DRAWER_H_

#include "rendering/core/draw_context.h"
class Editor;
class LiveSocket;

class LiveCursorDrawer {
public:
	void draw(const DrawContext& ctx, Editor& editor);
};

#endif
