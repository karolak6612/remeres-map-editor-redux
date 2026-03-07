//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "editor/editor.h"
#include "map/tile.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/view_state.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#ifndef RME_RENDERING_FLOOR_DRAWER_H_
#include "rendering/drawers/tiles/floor_drawer.h"
#endif
#include "rendering/drawers/tiles/tile_renderer.h"

FloorDrawer::FloorDrawer() {}

FloorDrawer::~FloorDrawer() {}

void FloorDrawer::draw(const DrawContext &ctx,
                       const FloorViewParams &floor_params,
                       const TileRenderContext &render_ctx) {

  // Draw "transparent higher floor"
  if (ctx.view.floor != 8 && ctx.view.floor != 0 &&
      ctx.options.settings.transparent_floors) {
    int map_z = ctx.view.floor - 1;
    for (int map_x = floor_params.start_x; map_x <= floor_params.end_x;
         map_x++) {
      for (int map_y = floor_params.start_y; map_y <= floor_params.end_y;
           map_y++) {
        Tile *tile = render_ctx.editor.map.getTile(map_x, map_y, map_z);
        if (tile) {
          int draw_x = 0;
          int draw_y = 0;
          ctx.view.getScreenPosition(map_x, map_y, map_z, draw_x, draw_y);

          // Position pos = tile->getPosition();

          if (tile->ground) {
            BlitItemParams params(tile, tile->ground.get(), ctx.options);
            params.alpha = 96;
            if (tile->isPZ()) {
              params.red = 128;
              params.green = 255;
              params.blue = 128;
            }
            render_ctx.item_drawer.BlitItem(ctx, &render_ctx.sprite_drawer,
                                            &render_ctx.creature_drawer, draw_x,
                                            draw_y, params);
          }
          if (ctx.view.zoom <= 10.0 ||
              !ctx.options.settings.hide_items_when_zoomed) {
            for (const auto &item : tile->items) {
              BlitItemParams params(tile, item.get(), ctx.options);
              params.alpha = 96;
              render_ctx.item_drawer.BlitItem(ctx, &render_ctx.sprite_drawer,
                                              &render_ctx.creature_drawer,
                                              draw_x, draw_y, params);
            }
          }
        }
      }
    }
  }
}
