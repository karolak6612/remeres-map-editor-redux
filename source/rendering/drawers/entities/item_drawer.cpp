//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <algorithm>
#undef min
#undef max

#include "game/complexitem.h"
#include "game/item.h"
#include "game/items.h"
#include "game/sprites.h"
#include "map/tile.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/sprite_database.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/utilities/pattern_calculator.h"
#include "ui/gui.h"

BlitItemParams::BlitItemParams(const Tile *t, Item *i, const DrawingOptions &o)
    : tile(t), item(i), options(&o) {
  if (t) {
    pos = t->getPosition();
  }
}

BlitItemParams::BlitItemParams(const Position &p, Item *i,
                               const DrawingOptions &o)
    : pos(p), item(i), options(&o) {}

ItemDrawer::ItemDrawer() {}

ItemDrawer::~ItemDrawer() {}

void ItemDrawer::BlitItem(const DrawContext &ctx, SpriteDrawer *sprite_drawer,
                          CreatureDrawer *creature_drawer, int &draw_x,
                          int &draw_y, const BlitItemParams &params) {
  const Position &pos = params.pos;
  Item *item = params.item;
  const Tile *tile = params.tile;
  const DrawingOptions &options = *params.options;
  bool ephemeral = params.ephemeral;
  int red = params.red;
  int green = params.green;
  int blue = params.blue;
  int alpha = params.alpha;
  const SpritePatterns *cached_patterns = params.patterns;

  const ItemType &it = params.item_type ? *params.item_type : item->getType();

  // Locked door indicator
  if (!options.settings.ingame && options.settings.highlight_locked_doors &&
      it.isDoor()) {
    bool locked = item->isLocked();

    // Door orientation: horizontal wall -> West border (south=true), vertical
    // wall -> North border (east=true)
    if (it.border_alignment == WALL_HORIZONTAL) {
      DrawDoorIndicator(locked, pos, true, false);
    } else if (it.border_alignment == WALL_VERTICAL) {
      DrawDoorIndicator(locked, pos, false, true);
    } else {
      // Center case for non-aligned doors
      DrawDoorIndicator(locked, pos, false, false);
    }
  }

  bool is_transient_selected =
      !ephemeral && options.frame.transient_selection_bounds &&
      options.frame.transient_selection_bounds->contains(pos.x, pos.y);
  if (!options.settings.ingame &&
      (item->isSelected() || is_transient_selected)) {
    red /= 2;
    blue /= 2;
    green /= 2;
  }

  uint32_t clientID = it.clientID;

  // Display invisible and invalid items
  // Ugly hacks. :)
  if (!options.settings.ingame && options.settings.show_tech_items) {
    // Red invalid client id
    if (it.id == 0) {
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                  DrawColor(red, 0, 0, alpha));
      return;
    }

    switch (it.clientID) {
    // Yellow invisible stairs tile (459)
    case 469:
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                  DrawColor(red, green, 0, (alpha * 171) >> 8));
      return;

    // Red invisible walkable tile (460)
    case 470:
    case 17970:
    case 20028:
    case 34168:
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                  DrawColor(red, 0, 0, (alpha * 171) >> 8));
      return;

    // Cyan invisible wall (1548)
    case 2187:
      sprite_drawer->glBlitSquare(ctx, draw_x, draw_y,
                                  DrawColor(0, green, blue, 80));
      return;

    default:
      break;
    }

    // primal light
    if ((it.clientID >= 39092 && it.clientID <= 39100) ||
        it.clientID == 39236 || it.clientID == 39367 || it.clientID == 39368) {
      clientID = g_items[SPRITE_LIGHTSOURCE].clientID;
      red = 0;
      alpha = 180;
    }
  }

  bool has_metadata = clientID > 0 &&
                      clientID < g_gui.sprites.getMetadataSpace().size() &&
                      clientID < g_gui.sprites.getAtlasCacheSpace().size();
  const SpriteMetadata *metadata =
      has_metadata ? &g_gui.sprites.getMetadataSpace()[clientID] : nullptr;

  // metaItem, sprite not found or not hidden
  if (it.isMetaItem() || metadata == nullptr ||
      (!ephemeral && it.pickupable && !options.settings.show_items)) {
    return;
  }

  // int screenx = draw_x - spr->getDrawOffset().first;
  // int screeny = draw_y - spr->getDrawOffset().second;
  // The original code modified draw_x/draw_y AFTER calculating screenx/screeny
  // using the original draw_x/draw_y.
  int screenx = draw_x - metadata->drawoffset_x;
  int screeny = draw_y - metadata->drawoffset_y;

  // Set the newd drawing height accordingly
  draw_x -= metadata->draw_height;
  draw_y -= metadata->draw_height;

  SpritePatterns patterns;
  if (cached_patterns && clientID == it.clientID) {
    patterns = *cached_patterns;
  } else {
    patterns = PatternCalculator::Calculate(metadata, it, item, tile, pos);
  }

  int subtype = patterns.subtype;
  int pattern_x = patterns.x;
  int pattern_y = patterns.y;
  int pattern_z = patterns.z;
  int frame = patterns.frame;

  if (!ephemeral && options.settings.transparent_items &&
      (!it.isGroundTile() || metadata->width > 1 || metadata->height > 1) &&
      !it.isSplash() &&
      (!it.isBorder || metadata->width > 1 || metadata->height > 1)) {
    alpha /= 2;
  }

  if (it.isPodium()) {
    Podium *podium = static_cast<Podium *>(item);
    if (!podium->hasShowPlatform() && !options.settings.ingame) {
      if (options.settings.show_tech_items) {
        alpha /= 2;
      } else {
        alpha = 0;
      }
    }
  }

  SpriteAtlasCache &atlas = g_gui.sprites.getAtlasCacheSpace()[clientID];

  if (metadata->width == 1 && metadata->height == 1 && metadata->layers == 1) {
    const AtlasRegion *region;
    if (subtype == -1 && pattern_x == 0 && pattern_y == 0 && pattern_z == 0 &&
        frame == 0) {
      region =
          atlas.getAtlasRegion(clientID, *metadata, 0, 0, 0, -1, 0, 0, 0, 0);
    } else {
      region = atlas.getAtlasRegion(clientID, *metadata, 0, 0, 0, subtype,
                                    pattern_x, pattern_y, pattern_z, frame);
    }

    if (region) {
#ifdef DEBUG
      // DEBUG: Check for mismatch on Item 369 using PRECISE sub-sprite ID
      if (item->getID() == 369) {
        // Use 0,0 as pattern coordinates for 1x1 items
        uint32_t precise_expected_id =
            atlas.getSpriteId(*metadata, frame, 0, 0);
        if (region->debug_sprite_id != 0 && precise_expected_id != 0 &&
            region->debug_sprite_id != precise_expected_id) {
          spdlog::error("SPRITE MISMATCH DETECTED: Item 369 (Expected Sprite "
                        "ID {}, Actual Region Owner {})",
                        precise_expected_id, region->debug_sprite_id);
        }
      }
#endif
      sprite_drawer->glBlitAtlasQuad(ctx, screenx, screeny, region,
                                     DrawColor(red, green, blue, alpha));
    }
  } else {
    for (int cx = 0; cx != metadata->width; cx++) {
      for (int cy = 0; cy != metadata->height; cy++) {
        for (int cf = 0; cf != metadata->layers; cf++) {
          const AtlasRegion *region =
              atlas.getAtlasRegion(clientID, *metadata, cx, cy, cf, subtype,
                                   pattern_x, pattern_y, pattern_z, frame);
          if (region) {
            sprite_drawer->glBlitAtlasQuad(ctx, screenx - cx * TILE_SIZE,
                                           screeny - cy * TILE_SIZE, region,
                                           DrawColor(red, green, blue, alpha));
          }
        }
      }
    }
  }

  if (it.isPodium()) {
    Podium *podium = static_cast<Podium *>(item);
    Outfit outfit = podium->getOutfit();
    if (!podium->hasShowOutfit()) {
      if (podium->hasShowMount()) {
        outfit.lookType = outfit.lookMount;
        outfit.lookHead = outfit.lookMountHead;
        outfit.lookBody = outfit.lookMountBody;
        outfit.lookLegs = outfit.lookMountLegs;
        outfit.lookFeet = outfit.lookMountFeet;
        outfit.lookAddon = 0;
        outfit.lookMount = 0;
      } else {
        outfit.lookType = 0;
      }
    }
    if (!podium->hasShowMount()) {
      outfit.lookMount = 0;
    }

    creature_drawer->BlitCreature(
        ctx, sprite_drawer, draw_x, draw_y, outfit,
        static_cast<Direction>(podium->getDirection()),
        CreatureDrawOptions{.color = DrawColor(red, green, blue, alpha)});
  }

  // draw wall hook
  if (!options.settings.ingame && options.settings.show_hooks &&
      (it.hookSouth || it.hookEast)) {
    DrawHookIndicator(it, pos);
  }

  // draw light color indicator
  if (!options.settings.ingame && options.settings.show_light_str) {
    const SpriteLight &light = item->getLight();
    if (light.intensity > 0) {
      glm::vec4 lc = colorFromEightBitNorm(light.color);
      uint8_t byteR = static_cast<uint8_t>(std::round(lc.r * 255.0f));
      uint8_t byteG = static_cast<uint8_t>(std::round(lc.g * 255.0f));
      uint8_t byteB = static_cast<uint8_t>(std::round(lc.b * 255.0f));
      uint8_t byteA = 255;

      int startOffset = std::max<int>(16, 32 - light.intensity);
      int sqSize = TILE_SIZE - startOffset;

      // We need to disable texture 2d for BlitSquare.
      // SpriteDrawer::glBlitSquare does NOT disable texture 2d automatically?
      // SpriteDrawer::glBlitSquare internally uses BatchRenderer::DrawQuad
      // which sets blank texture if needed. So we don't need manual
      // enable/disable here anymore.

      sprite_drawer->glBlitSquare(ctx, draw_x + startOffset - 2,
                                  draw_y + startOffset - 2,
                                  DrawColor(0, 0, 0, byteA), sqSize + 2);
      sprite_drawer->glBlitSquare(
          ctx, draw_x + startOffset - 1, draw_y + startOffset - 1,
          DrawColor(byteR, byteG, byteB, byteA), sqSize);
    }
  }
}

void ItemDrawer::DrawRawBrush(const DrawContext &ctx,
                              SpriteDrawer *sprite_drawer, int screenx,
                              int screeny, ItemType *itemType, uint8_t r,
                              uint8_t g, uint8_t b, uint8_t alpha) {
  uint32_t clientID = itemType->clientID;
  uint16_t cid = itemType->clientID;

  switch (cid) {
  // Yellow invisible stairs tile
  case 469:
    b = 0;
    alpha = (alpha * 171) >> 8;
    clientID = g_items[SPRITE_ZONE].clientID;
    break;

  // Red invisible walkable tile
  case 470:
    g = 0;
    b = 0;
    alpha = (alpha * 171) >> 8;
    clientID = g_items[SPRITE_ZONE].clientID;
    break;

  // Cyan invisible wall
  case 2187:
    r = 0;
    alpha = alpha / 3;
    clientID = g_items[SPRITE_ZONE].clientID;
    break;

  default:
    break;
  }

  // primal light
  if ((cid >= 39092 && cid <= 39100) || cid == 39236 || cid == 39367 ||
      cid == 39368) {
    clientID = g_items[SPRITE_LIGHTSOURCE].clientID;
    r = 0;
    alpha = (alpha * 171) >> 8;
  }

  sprite_drawer->BlitSprite(ctx, screenx, screeny, clientID,
                            DrawColor(r, g, b, alpha));
}

void ItemDrawer::DrawHookIndicator(const ItemType &type, const Position &pos) {
  if (hook_indicator_drawer) {
    hook_indicator_drawer->addHook(pos, type.hookSouth, type.hookEast);
  }
}

void ItemDrawer::DrawDoorIndicator(bool locked, const Position &pos, bool south,
                                   bool east) {
  if (door_indicator_drawer) {
    door_indicator_drawer->addDoor(pos, locked, south, east);
  }
}
