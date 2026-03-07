#include "app/main.h"

// glut include removed

#include "brushes/brush.h"
#include "editor/copybuffer.h"
#include "editor/editor.h"
#include "rendering/core/canvas_state.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/view_state.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/overlays/preview_drawer.h"
#include "rendering/ui/map_display.h"
#include "ui/gui.h"
#include "ui/map_tab.h"

PreviewDrawer::PreviewDrawer() {}

PreviewDrawer::~PreviewDrawer() {}

void PreviewDrawer::draw(const DrawContext &ctx,
                         const FloorViewParams &floor_params, int map_z,
                         Map &map, CopyBuffer &copybuffer, ItemDrawer *item_drawer,
                         SpriteDrawer *sprite_drawer,
                         CreatureDrawer *creature_drawer) {
  const BaseMap *secondary_map = ctx.state.canvas_state.secondary_map;

  if (secondary_map != nullptr && !ctx.state.options.settings.ingame) {
    Brush *brush = g_gui.GetCurrentBrush();

    Position normalPos;
    Position to(ctx.state.view.mouse_map_x, ctx.state.view.mouse_map_y, ctx.state.view.floor);

    if (ctx.state.canvas_state.is_pasting) {
      normalPos = copybuffer.getPosition();
    } else if (brush && brush->is<DoodadBrush>()) {
      normalPos = Position(0x8000, 0x8000, 0x8);
    } else {
      normalPos = to;
    }

    for (int map_x = floor_params.start_x; map_x <= floor_params.end_x;
         map_x++) {
      for (int map_y = floor_params.start_y; map_y <= floor_params.end_y;
           map_y++) {
        Position final(map_x, map_y, map_z);
        Position pos = normalPos + final - to;

        if (pos.z >= MAP_LAYERS || pos.z < 0) {
          continue;
        }

        const Tile *tile = secondary_map->getTile(pos);
        if (tile) {
          int draw_x, draw_y;
          ctx.state.view.getScreenPosition(map_x, map_y, map_z, draw_x, draw_y);

          uint8_t base_alpha = ctx.state.canvas_state.is_pasting ? 128 : 255;
          drawTilePreview(ctx, draw_x, draw_y, tile, base_alpha, item_drawer,
                          sprite_drawer, creature_drawer);
        }
      }
    }
    // Draw highlight on the specific tile under mouse
    // This helps user see where they are pointing in the "chaos"
    Position mousePos(ctx.state.view.mouse_map_x, ctx.state.view.mouse_map_y,
                      ctx.state.view.floor);
    if (mousePos.z == map_z) {
      int draw_x, draw_y;
      ctx.state.view.getScreenPosition(mousePos.x, mousePos.y, mousePos.z, draw_x,
                                 draw_y);

      if (g_gui.atlas.ensureAtlasManager()) {
        // Draw a semi-transparent white box over the tile
        glm::vec4 highlightColor(1.0f, 1.0f, 1.0f, 0.25f); // 25% white
        ctx.backend.sprite_batch.drawRect(
            (float)draw_x, (float)draw_y, (float)TILE_SIZE, (float)TILE_SIZE,
            highlightColor, *g_gui.atlas.getAtlasManager());
      }
    }
  }
}

void PreviewDrawer::drawTilePreview(const DrawContext &ctx, int draw_x,
                                    int draw_y, const Tile *tile,
                                    uint8_t base_alpha, ItemDrawer *item_drawer,
                                    SpriteDrawer *sprite_drawer,
                                    CreatureDrawer *creature_drawer) const {
  uint8_t r = 255, g = 255, b = 255;

  if (tile->ground) {
    if (tile->isBlocking() && ctx.state.options.settings.show_blocking) {
      g = g / 3 * 2;
      b = b / 3 * 2;
    }
    if (tile->isHouseTile() && ctx.state.options.settings.show_houses) {
      if (tile->getHouseID() == ctx.state.canvas_state.current_house_id) {
        r /= 2;
      } else {
        r /= 2;
        g /= 2;
      }
    } else if (ctx.state.options.settings.show_special_tiles && tile->isPZ()) {
      r /= 2;
      b /= 2;
    }
    if (ctx.state.options.settings.show_special_tiles &&
        tile->getMapFlags() & TILESTATE_PVPZONE) {
      uint8_t r_orig = r;
      r = r / 3 * 2;
      b = r_orig / 3 * 2;
    }
    if (ctx.state.options.settings.show_special_tiles &&
        tile->getMapFlags() & TILESTATE_NOLOGOUT) {
      b /= 2;
    }
    if (ctx.state.options.settings.show_special_tiles &&
        tile->getMapFlags() & TILESTATE_NOPVP) {
      g /= 2;
    }

    BlitItemParams params(tile, tile->ground.get(), ctx.state.options);
    params.ephemeral = true;
    params.red = r;
    params.green = g;
    params.blue = b;
    params.alpha = base_alpha;
    item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x, draw_y,
                          params);
  }

  if (ctx.state.view.zoom <= 10.0 || !ctx.state.options.settings.hide_items_when_zoomed) {
    for (const auto &item : tile->items) {
      BlitItemParams params(tile, item.get(), ctx.state.options);
      params.ephemeral = true;
      params.alpha = base_alpha;
      if (item->isBorder()) {
        params.red = 255;
        params.green = r;
        params.blue = g;
        params.alpha = (base_alpha == 255) ? b : base_alpha;
        item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                              draw_y, params);
      } else {
        item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                              draw_y, params);
      }
    }
    if (tile->creature && ctx.state.options.settings.show_creatures) {
      creature_drawer->BlitCreature(ctx, sprite_drawer, draw_x, draw_y,
                                    tile->creature.get());
    }
  }
}
