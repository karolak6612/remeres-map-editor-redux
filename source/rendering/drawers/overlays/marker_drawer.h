#ifndef RME_MARKER_DRAWER_H_
#define RME_MARKER_DRAWER_H_

#include "rendering/core/drawing_options.h"
#include <cstdint>

class SpriteDrawer;
class Tile;
class Waypoint;

class Editor;
class SpriteBatch;
class VisualOverlayDrawer;

class MarkerDrawer {
public:
	MarkerDrawer();
	~MarkerDrawer();

	void draw(SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const Tile* tile, Waypoint* waypoint, uint32_t current_house_id, Editor& editor, const DrawingOptions& options);
	void SetVisualOverlayDrawer(VisualOverlayDrawer* drawer) {
		visual_overlay_drawer = drawer;
	}

private:
	VisualOverlayDrawer* visual_overlay_drawer = nullptr;
};

#endif
