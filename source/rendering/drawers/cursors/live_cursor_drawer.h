#ifndef RME_RENDERING_LIVE_CURSOR_DRAWER_H_
#define RME_RENDERING_LIVE_CURSOR_DRAWER_H_

struct DrawContext;
class IMapAccess;

class LiveCursorDrawer {
public:
	void draw(const DrawContext& ctx, IMapAccess& map_access);
};

#endif
