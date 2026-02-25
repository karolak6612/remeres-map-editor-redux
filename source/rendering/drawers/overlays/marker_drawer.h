#ifndef RME_MARKER_DRAWER_H_
#define RME_MARKER_DRAWER_H_

#include "rendering/core/drawing_options.h"
#include <cstdint>

class SpriteBatch;
class SpriteDrawer;
class RenderList;
struct DrawingOptions;

struct MarkerFlags {
	bool has_waypoint = false;
	bool is_house_exit = false;
	bool has_house_exit_match = false;
	bool is_town_exit = false;
	bool has_spawn = false;
	bool is_spawn_selected = false;
};

class MarkerDrawer {
public:
	MarkerDrawer();
	~MarkerDrawer();

	void draw(SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerFlags& flags, const DrawingOptions& options);
	void draw(RenderList& list, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerFlags& flags, const DrawingOptions& options);
};

#endif
