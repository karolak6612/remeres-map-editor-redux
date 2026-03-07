#ifndef RME_RENDERING_LIVE_CURSOR_DRAWER_H_
#define RME_RENDERING_LIVE_CURSOR_DRAWER_H_

#include "rendering/core/draw_context.h"
class LiveManager;
class LiveSocket;

class LiveCursorDrawer {
public:
	void draw(const DrawContext& ctx, LiveManager& live_manager);
};

#endif
