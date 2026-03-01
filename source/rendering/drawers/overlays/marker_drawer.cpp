#include "rendering/drawers/overlays/marker_drawer.h"
#include "app/main.h"
#include "editor/editor.h"
#include "game/sprites.h"
#include "map/tile.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/entities/sprite_drawer.h"


MarkerDrawer::MarkerDrawer() {}

MarkerDrawer::~MarkerDrawer() {}

void MarkerDrawer::draw(const DrawContext &ctx, SpriteDrawer *drawer,
                        int draw_x, int draw_y, const Tile *tile,
                        Waypoint *waypoint, uint32_t current_house_id,
                        Editor &editor) {
  // waypoint (blue flame)
  if (!ctx.options.ingame && waypoint && ctx.options.show_waypoints) {
    drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_WAYPOINT,
                       DrawColor(64, 64, 255));
  }

  // house exit (blue splash)
  if (tile->isHouseExit() && ctx.options.show_houses) {
    if (tile->hasHouseExit(current_house_id)) {
      drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_HOUSE_EXIT,
                         DrawColor(64, 255, 255));
    } else {
      drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_HOUSE_EXIT,
                         DrawColor(64, 64, 255));
    }
  }

  // town temple (gray flag)
  if (ctx.options.show_towns && tile->isTownExit(editor.map)) {
    drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_TOWN_TEMPLE,
                       DrawColor(255, 255, 64, 170));
  }

  // spawn (purple flame)
  if (tile->spawn && ctx.options.show_spawns) {
    if (tile->spawn->isSelected()) {
      drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_SPAWN,
                         DrawColor(128, 128, 128));
    } else {
      drawer->BlitSprite(ctx, draw_x, draw_y, SPRITE_SPAWN,
                         DrawColor(255, 255, 255));
    }
  }
}
