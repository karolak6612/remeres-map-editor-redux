#include "app/main.h"

// glut include removed

#include "brushes/brush.h"
#include "editor/copybuffer.h"
#include "editor/editor.h"
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
                         const FloorViewParams &floor_params, MapCanvas *canvas,
                         int map_z, Editor &editor, ItemDrawer *item_drawer,
                         SpriteDrawer *sprite_drawer,
                         CreatureDrawer *creature_drawer,
                         uint32_t current_house_id) {
  MapTab *mapTab = dynamic_cast<MapTab *>(canvas->GetMapWindow());
  BaseMap *secondary_map =
      mapTab ? mapTab->GetSession()->secondary_map : nullptr;

  if (secondary_map != nullptr && !ctx.options.ingame) {
    Brush *brush = g_gui.GetCurrentBrush();

    Position normalPos;
    Position to(ctx.view.mouse_map_x, ctx.view.mouse_map_y, ctx.view.floor);

    if (canvas->isPasting()) {
      normalPos = editor.copybuffer.getPosition();
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

        Tile *tile = secondary_map->getTile(pos);
        if (tile) {
          int draw_x, draw_y;
          ctx.view.getScreenPosition(map_x, map_y, map_z, draw_x, draw_y);

          // Draw ground
          uint8_t r = 255, g = 255, b = 255;
          uint8_t base_alpha = canvas->isPasting() ? 128 : 255;

          if (tile->ground) {
            if (tile->isBlocking() && ctx.options.show_blocking) {
              g = g / 3 * 2;
              b = b / 3 * 2;
            }
            if (tile->isHouseTile() && ctx.options.show_houses) {
              if ((int)tile->getHouseID() == current_house_id) {
                r /= 2;
              } else {
                r /= 2;
                g /= 2;
              }
            } else if (ctx.options.show_special_tiles && tile->isPZ()) {
              r /= 2;
              b /= 2;
            }
            if (ctx.options.show_special_tiles &&
                tile->getMapFlags() & TILESTATE_PVPZONE) {
              r = r / 3 * 2;
              b = r / 3 * 2;
            }
            if (ctx.options.show_special_tiles &&
                tile->getMapFlags() & TILESTATE_NOLOGOUT) {
              b /= 2;
            }
            if (ctx.options.show_special_tiles &&
                tile->getMapFlags() & TILESTATE_NOPVP) {
              g /= 2;
            }
            if (tile->ground) {
              BlitItemParams params(tile, tile->ground.get(), ctx.options);
              params.ephemeral = true;
              params.red = r;
              params.green = g;
              params.blue = b;
              params.alpha = base_alpha;
              item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                                    draw_y, params);
            }
          }

          // Draw items on the tile
          if (ctx.view.zoom <= 10.0 || !ctx.options.hide_items_when_zoomed) {
            for (const auto &item : tile->items) {
              BlitItemParams params(tile, item.get(), ctx.options);
              params.ephemeral = true;
              params.alpha = base_alpha;
              if (item->isBorder()) {
                params.red = 255;
                params.green = r;
                params.blue = g;
                params.alpha = (base_alpha == 255) ? b : base_alpha;
                item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer,
                                      draw_x, draw_y, params);
              } else {
                item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer,
                                      draw_x, draw_y, params);
              }
            }
            if (tile->creature && ctx.options.show_creatures) {
              creature_drawer->BlitCreature(ctx, sprite_drawer, draw_x, draw_y,
                                            tile->creature.get());
            }
          }
        }
      }
    }
    // Draw highlight on the specific tile under mouse
    // This helps user see where they are pointing in the "chaos"
    Position mousePos(ctx.view.mouse_map_x, ctx.view.mouse_map_y,
                      ctx.view.floor);
    if (mousePos.z == map_z) {
      int draw_x, draw_y;
      ctx.view.getScreenPosition(mousePos.x, mousePos.y, mousePos.z, draw_x,
                                 draw_y);

      if (g_gui.atlas_lifecycle.ensureAtlasManager()) {
        // Draw a semi-transparent white box over the tile
        glm::vec4 highlightColor(1.0f, 1.0f, 1.0f, 0.25f); // 25% white
        ctx.sprite_batch.drawRect(
            (float)draw_x, (float)draw_y, (float)TILE_SIZE, (float)TILE_SIZE,
            highlightColor, *g_gui.atlas_lifecycle.getAtlasManager());
      }
    }
  }
}
