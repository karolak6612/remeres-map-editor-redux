#include "app/main.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/tile_render_snapshot.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "game/sprites.h"

MarkerDrawer::MarkerDrawer() {
}

MarkerDrawer::~MarkerDrawer() {
}

void MarkerDrawer::draw(
	SpriteBatch& sprite_batch, SpriteDrawer* drawer, int draw_x, int draw_y, const MarkerRenderSnapshot& marker, const RenderSettings& settings
) {
	// waypoint (blue flame)
	if (!settings.ingame && marker.has_waypoint && settings.show_waypoints) {
		drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_WAYPOINT, DrawColor(64, 64, 255));
	}

	// house exit (blue splash)
	if (marker.is_house_exit && settings.show_houses) {
		if (marker.is_current_house_exit) {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 255, 255));
		} else {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_HOUSE_EXIT, DrawColor(64, 64, 255));
		}
	}

	// town temple (gray flag)
	if (settings.show_towns && marker.is_town_exit) {
		drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_TOWN_TEMPLE, DrawColor(255, 255, 64, 170));
	}

	// spawn (purple flame)
	if (marker.has_spawn && settings.show_spawns) {
		if (marker.spawn_selected) {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_SPAWN, DrawColor(128, 128, 128));
		} else {
			drawer->BlitSprite(sprite_batch, draw_x, draw_y, SPRITE_SPAWN, DrawColor(255, 255, 255));
		}
	}
}
