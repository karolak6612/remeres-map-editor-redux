#include "rendering/drawers/tiles/tile_renderer.h"
#include "app/main.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "ui/gui.h"

#include "brushes/waypoint/waypoint_brush.h"
#include "editor/editor.h"
#include "game/complexitem.h"
#include "game/item.h"
#include "game/items.h"
#include "game/sprites.h"
#include "map/tile.h"

#include "app/definitions.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/sprite_atlas_cache.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/core/view_state.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"

#include "rendering/core/light_buffer.h"
#include "rendering/core/sprite_preloader.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/utilities/pattern_calculator.h"
#include "rendering/ui/tooltip_data_extractor.h"
TileRenderer::TileRenderer(const TileRenderContext& ctx)
    : item_drawer(&ctx.item_drawer),
      sprite_drawer(&ctx.sprite_drawer),
      creature_drawer(&ctx.creature_drawer),
      floor_drawer(&ctx.floor_drawer),
      marker_drawer(&ctx.marker_drawer),
      tooltip_drawer(&ctx.tooltip_drawer),
      creature_name_drawer(&ctx.creature_name_drawer),
      editor(&ctx.editor) {}

void TileRenderer::DrawTile(const DrawContext &ctx, TileLocation *location,
                            uint32_t current_house_id, int in_draw_x,
                            int in_draw_y, bool draw_lights) {
  if (!location) {
    return;
  }
  Tile *tile = location->get();

  if (!tile) {
    return;
  }

  if (ctx.options.settings.show_only_modified && !tile->isModified()) {
    return;
  }

  int map_x = location->getX();
  int map_y = location->getY();
  int map_z = location->getZ();

  int draw_x, draw_y;
  if (in_draw_x != -1 && in_draw_y != -1) {
    draw_x = in_draw_x;
    draw_y = in_draw_y;
  } else {
    // Early viewport culling - skip tiles that are completely off-screen
    if (!ctx.view.IsTileVisible(map_x, map_y, map_z, draw_x, draw_y)) {
      return;
    }
  }

  const auto &position = location->getPosition();

  // Light Processing (Ground)
  if (draw_lights && tile->hasLight()) {
    if (tile->ground && tile->ground->hasLight()) {
      ctx.light_buffer.AddLight(position.x, position.y, position.z,
                                tile->ground->getLight());
    }
  }

  Waypoint *waypoint = nullptr;
  if (location->getWaypointCount() > 0) {
    waypoint = editor->map.waypoints.getWaypoint(location);
  }

  // Waypoint tooltip (one per waypoint)
  if (ctx.options.settings.show_tooltips && waypoint &&
      map_z == ctx.view.floor) {
    tooltip_drawer->addWaypointTooltip(position, waypoint->name);
  }

  bool as_minimap = ctx.options.settings.show_as_minimap;
  bool only_colors = as_minimap || ctx.options.settings.show_only_colors;

  uint8_t r = 255, g = 255, b = 255;

  // begin filters for ground tile
  if (!as_minimap) {
    TileColorCalculator::Calculate(tile, ctx.options, current_house_id,
                                   location->getSpawnCount(), r, g, b);
  }

  const ItemType *ground_it = nullptr;
  if (tile->ground) {
    ground_it = &tile->ground->getType();
  }

  if (only_colors) {
    if (as_minimap) {
      TileColorCalculator::GetMinimapColor(tile, r, g, b);
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y, DrawColor(r, g, b, 255));
    } else if (r != 255 || g != 255 || b != 255) {
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y, DrawColor(r, g, b, 128));
    }
  } else {
    if (tile->ground && ground_it) {
      uint32_t clientID = ground_it->clientID;
      bool has_metadata = clientID > 0 &&
                          clientID < g_gui.sprites.getMetadataSpace().size() &&
                          clientID < g_gui.sprites.getAtlasCacheSpace().size();
      if (has_metadata) {
        const SpriteMetadata &metadata =
            g_gui.sprites.getMetadataSpace()[clientID];
        SpriteAtlasCache &atlas = g_gui.sprites.getAtlasCacheSpace()[clientID];
        SpritePatterns patterns = PatternCalculator::Calculate(
            &metadata, *ground_it, tile->ground.get(), tile, position);

        // Inline preload check — skip function call when sprite is simple and
        // loaded (95%+ case)
        if (!atlas.isSimpleAndLoaded(metadata)) {
          rme::collectTileSprites(g_gui.atlas.getSpritePreloader(), clientID,
                                  patterns.x, patterns.y, patterns.z,
                                  patterns.frame);
        }

        BlitItemParams params(position, tile->ground.get(), ctx.options);
        params.tile = tile;
        params.item_type = ground_it;
        params.red = r;
        params.green = g;
        params.blue = b;
        params.patterns = &patterns;
        item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                              draw_y, params);
      } else {
        // Missing sprite — draw magenta placeholder
        sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                    DrawColor(255, 0, 255, 128));
      }
    } else if (ctx.options.settings.always_show_zones &&
               (r != 255 || g != 255 || b != 255)) {
      ItemType *zoneItem = &g_items[SPRITE_ZONE];
      item_drawer->DrawRawBrush(ctx, sprite_drawer, draw_x, draw_y, zoneItem, r,
                                g, b, 60);
    }
  }

  // Cache isHouseTile — used multiple times below
  const bool is_house_tile = tile->isHouseTile();

  // Ground tooltip (one per item)
  if (ctx.options.settings.show_tooltips && map_z == ctx.view.floor &&
      tile->ground && ground_it) {
    TooltipData &groundData = tooltip_drawer->requestTooltipData();
    if (rme::FillItemTooltipData(groundData, tile->ground.get(), *ground_it,
                            position, is_house_tile, ctx.view.zoom)) {
      if (groundData.hasVisibleFields()) {
        tooltip_drawer->commitTooltip();
      }
    }
  }

  // end filters for ground tile

  // Draw helper border for selected house tiles
  // Only draw on the current floor (grid)
  if (ctx.options.settings.show_houses && is_house_tile &&
      static_cast<int>(tile->getHouseID()) == current_house_id &&
      map_z == ctx.view.floor) {

    uint8_t hr, hg, hb;
    TileColorCalculator::GetHouseColor(tile->getHouseID(), hr, hg, hb);

    float intensity = 0.5f + (0.5f * ctx.options.frame.highlight_pulse);
    // Optimization: Use integer math for border color to avoid vec4
    // construction and casting
    int ba = static_cast<int>(intensity * 255.0f);
    // hr, hg, hb are already uint8_t
    sprite_drawer->glDrawBox(ctx, draw_x, draw_y, 32, 32,
                             DrawColor(hr, hg, hb, ba));
  }

  if (!only_colors) {
    if (ctx.view.zoom < 10.0 || !ctx.options.settings.hide_items_when_zoomed) {
      // Hoist house color calculation out of item loop
      uint8_t house_r = 255, house_g = 255, house_b = 255;
      bool calculate_house_color = ctx.options.settings.extended_house_shader &&
                                   ctx.options.settings.show_houses &&
                                   is_house_tile;
      bool should_pulse =
          calculate_house_color &&
          (static_cast<int>(tile->getHouseID()) == current_house_id) &&
          (ctx.options.frame.highlight_pulse > 0.0f);
      float boost = 0.0f;

      if (calculate_house_color) {
        TileColorCalculator::GetHouseColor(tile->getHouseID(), house_r, house_g,
                                           house_b);
        if (should_pulse) {
          boost = ctx.options.frame.highlight_pulse * 0.6f;
        }
      }

      bool process_tooltips =
          ctx.options.settings.show_tooltips && map_z == ctx.view.floor;

      for (const auto &item : tile->items) {
        if (draw_lights && item->hasLight()) {
          ctx.light_buffer.AddLight(position.x, position.y, position.z,
                                    item->getLight());
        }

        const ItemType &it = item->getType();

        if (process_tooltips) {
          TooltipData &itemData = tooltip_drawer->requestTooltipData();
          if (rme::FillItemTooltipData(itemData, item.get(), it, position,
                                  is_house_tile, ctx.view.zoom)) {
            if (itemData.hasVisibleFields()) {
              tooltip_drawer->commitTooltip();
            }
          }
        }

        uint32_t clientID = it.clientID;
        bool has_metadata =
            clientID > 0 &&
            clientID < g_gui.sprites.getMetadataSpace().size() &&
            clientID < g_gui.sprites.getAtlasCacheSpace().size();

        if (has_metadata) {
          const SpriteMetadata &metadata =
              g_gui.sprites.getMetadataSpace()[clientID];
          SpriteAtlasCache &atlas =
              g_gui.sprites.getAtlasCacheSpace()[clientID];

          SpritePatterns patterns = PatternCalculator::Calculate(
              &metadata, it, item.get(), tile, position);

          // Inline preload check — skip function call when sprite is simple and
          // loaded
          if (!atlas.isSimpleAndLoaded(metadata)) {
            rme::collectTileSprites(g_gui.atlas.getSpritePreloader(), clientID,
                                    patterns.x, patterns.y, patterns.z,
                                    patterns.frame);
          }

          BlitItemParams params(position, item.get(), ctx.options);
          params.tile = tile;
          params.item_type = &it;
          params.patterns = &patterns;

          // item sprite
          if (item->isBorder()) {
            params.red = r;
            params.green = g;
            params.blue = b;
            item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                                  draw_y, params);
          } else {
            uint8_t ir = 255, ig = 255, ib = 255;

            if (calculate_house_color) {
              // Apply house color tint
              ir = static_cast<uint8_t>(ir * house_r / 255);
              ig = static_cast<uint8_t>(ig * house_g / 255);
              ib = static_cast<uint8_t>(ib * house_b / 255);

              if (should_pulse) {
                // Pulse effect matching the tile pulse
                ir = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(ir + (255 - ir) * boost)));
                ig = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(ig + (255 - ig) * boost)));
                ib = static_cast<uint8_t>(
                    std::min(255, static_cast<int>(ib + (255 - ib) * boost)));
              }
            }
            params.red = ir;
            params.green = ig;
            params.blue = ib;
            item_drawer->BlitItem(ctx, sprite_drawer, creature_drawer, draw_x,
                                  draw_y, params);
          }
        } else if (clientID > 0) {
          // Missing sprite — draw magenta placeholder
          sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                      DrawColor(255, 0, 255, 128));
        }
      }
      // monster/npc on tile
      if (tile->creature && ctx.options.settings.show_creatures) {
        creature_drawer->BlitCreature(
            ctx, sprite_drawer, draw_x, draw_y, tile->creature.get(),
            CreatureDrawOptions{
                .map_pos = position,
                .transient_selection_bounds =
                    ctx.options.frame.transient_selection_bounds});
        if (creature_name_drawer) {
          creature_name_drawer->addLabel(position, tile->creature->getName(),
                                         tile->creature.get());
        }
      }
    }

    if (ctx.view.zoom < 10.0) {
      // markers (waypoint, house exit, town temple, spawn)
      marker_drawer->draw(ctx, sprite_drawer, draw_x, draw_y, tile, waypoint,
                          current_house_id, *editor);
    }
  }
}

void TileRenderer::PreloadItem(const Tile *tile, Item *item, const ItemType &it,
                               const SpritePatterns *cached_patterns) {
  if (!item) {
    return;
  }

  uint32_t clientID = it.clientID;
  bool has_metadata = clientID > 0 &&
                      clientID < g_gui.sprites.getMetadataSpace().size() &&
                      clientID < g_gui.sprites.getAtlasCacheSpace().size();

  if (has_metadata) {
    const SpriteMetadata &metadata = g_gui.sprites.getMetadataSpace()[clientID];
    SpriteAtlasCache &atlas = g_gui.sprites.getAtlasCacheSpace()[clientID];

    if (!atlas.isSimpleAndLoaded(metadata)) {
      SpritePatterns patterns;
      if (cached_patterns) {
        patterns = *cached_patterns;
      } else {
        patterns = PatternCalculator::Calculate(&metadata, it, item, tile,
                                                tile->getPosition());
      }
      rme::collectTileSprites(g_gui.atlas.getSpritePreloader(), clientID,
                              patterns.x, patterns.y, patterns.z,
                              patterns.frame);
    }
  }
}
