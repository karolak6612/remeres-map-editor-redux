#include "app/main.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/core/graphics.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/render_list.h"
#include "map/tile.h"
#include "game/sprites.h"
#include "editor/editor.h"

MarkerDrawer::MarkerDrawer() {
}

MarkerDrawer::~MarkerDrawer() {
}

void MarkerDrawer::draw(SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerFlags& flags, const DrawingOptions& options) {
	// waypoint (blue flame)
	if (!options.ingame && flags.has_waypoint && options.show_waypoints) {
		drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_WAYPOINT, DrawColor(64, 64, 255));
	}

	// house exit (blue splash)
	if (flags.is_house_exit && options.show_houses) {
		if (flags.has_house_exit_match) {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 255, 255));
		} else {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 64, 255));
		}
	}

	// town temple (gray flag)
	if (options.show_towns && flags.is_town_exit) {
		drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_TOWN_TEMPLE, DrawColor(255, 255, 64, 170));
	}

	// spawn (purple flame)
	if (flags.has_spawn && options.show_spawns) {
		if (flags.is_spawn_selected) {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_SPAWN, DrawColor(128, 128, 128));
		} else {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_SPAWN, DrawColor(255, 255, 255));
		}
	}
}

void MarkerDrawer::draw(RenderList& list, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerFlags& flags, const DrawingOptions& options) {
	if (!options.ingame && flags.has_waypoint && options.show_waypoints) {
		drawer->BlitSprite(list, draw_x, draw_y, SPRITE_WAYPOINT, DrawColor(64, 64, 255));
	}

	if (flags.is_house_exit && options.show_houses) {
		if (flags.has_house_exit_match) {
			drawer->BlitSprite(list, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 255, 255));
		} else {
			drawer->BlitSprite(list, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 64, 255));
		}
	}

	if (options.show_towns && flags.is_town_exit) {
		drawer->BlitSprite(list, draw_x, draw_y, SPRITE_TOWN_TEMPLE, DrawColor(255, 255, 64, 170));
	}

	if (flags.has_spawn && options.show_spawns) {
		if (flags.is_spawn_selected) {
			drawer->BlitSprite(list, draw_x, draw_y, SPRITE_SPAWN, DrawColor(128, 128, 128));
		} else {
			drawer->BlitSprite(list, draw_x, draw_y, SPRITE_SPAWN, DrawColor(255, 255, 255));
		}
	}
}
