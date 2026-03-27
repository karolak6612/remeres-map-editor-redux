#ifndef RME_LUA_OVERLAY_DRAWER_H_
#define RME_LUA_OVERLAY_DRAWER_H_

#include "rendering/core/drawing_options.h"
#include "rendering/core/map_overlay.h"
#include "rendering/core/render_view.h"

#include <vector>

class MapDrawer;
class CoordinateMapper;

struct NVGcontext;

class LuaOverlayDrawer {
public:
	LuaOverlayDrawer(MapDrawer* mapDrawer);
	~LuaOverlayDrawer();

	void Draw(const RenderView& view, const DrawingOptions& options);
	void DrawUI(NVGcontext* vg, const RenderView& view, const DrawingOptions& options);

private:
	struct CacheKey {
		int start_x = 0;
		int start_y = 0;
		int end_x = 0;
		int end_y = 0;
		int floor = 0;
		double zoom = 0.0;
		int view_scroll_x = 0;
		int view_scroll_y = 0;
		int tile_size = 0;
		int screen_width = 0;
		int screen_height = 0;
	};

	MapDrawer* mapDrawer;
	std::vector<MapOverlayCommand> cachedCommands;
	CacheKey cachedKey {};
	bool cacheValid = false;

	void refreshCache(const RenderView& view);
	CacheKey makeCacheKey(const RenderView& view) const;
};

#endif
