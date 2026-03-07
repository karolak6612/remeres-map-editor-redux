#include "rendering/drawers/overlays/marker_drawer.h"
#include "app/main.h"
#include "editor/editor.h"
#include "game/items.h"
#include "game/sprites.h"
#include "map/tile.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/texture_gc.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/io/sprite_loader.h"

MarkerDrawer::MarkerDrawer() {}

MarkerDrawer::~MarkerDrawer() {}

void MarkerDrawer::draw(const DrawContext &ctx, SpriteDrawer *drawer,
                        int draw_x, int draw_y, const Tile *tile,
                        Waypoint *waypoint, uint32_t current_house_id,
                        Map &map) {
  // waypoint (blue flame)
  if (!ctx.state.options.settings.ingame && waypoint &&
      ctx.state.options.settings.show_waypoints) {
    drawer->BlitSprite(ctx, draw_x, draw_y, g_items[SPRITE_WAYPOINT].clientID,
                       DrawColor(64, 64, 255));
  }

  // house exit (blue splash)
  if (tile->isHouseExit() && ctx.state.options.settings.show_houses) {
    if (tile->hasHouseExit(current_house_id)) {
      drawer->BlitSprite(ctx, draw_x, draw_y,
                         g_items[SPRITE_HOUSE_EXIT].clientID,
                         DrawColor(64, 255, 255));
    } else {
      drawer->BlitSprite(ctx, draw_x, draw_y,
                         g_items[SPRITE_HOUSE_EXIT].clientID,
                         DrawColor(64, 64, 255));
    }
  }

  // town temple (gray flag)
  if (ctx.state.options.settings.show_towns && tile->isTownExit(map)) {
    drawer->BlitSprite(ctx, draw_x, draw_y,
                       g_items[SPRITE_TOWN_TEMPLE].clientID,
                       DrawColor(255, 255, 64, 170));
  }

  // spawn (purple flame)
  if (tile->spawn && ctx.state.options.settings.show_spawns) {
    if (tile->spawn->isSelected()) {
      drawer->BlitSprite(ctx, draw_x, draw_y, g_items[SPRITE_SPAWN].clientID,
                         DrawColor(128, 128, 128));
    } else {
      drawer->BlitSprite(ctx, draw_x, draw_y, g_items[SPRITE_SPAWN].clientID,
                         DrawColor(255, 255, 255));
    }
  }
}
