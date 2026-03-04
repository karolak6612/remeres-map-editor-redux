#ifndef RME_PREVIEW_DRAWER_H_
#define RME_PREVIEW_DRAWER_H_

#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"
#include <cstdint>

class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class Tile;

class PreviewDrawer {
public:
  PreviewDrawer();
  ~PreviewDrawer();

  void draw(const DrawContext &ctx, const FloorViewParams &floor_params,
            int map_z, Editor &editor, ItemDrawer *item_drawer,
            SpriteDrawer *sprite_drawer, CreatureDrawer *creature_drawer);

private:
  void drawTilePreview(const DrawContext &ctx, int draw_x, int draw_y,
                       Tile *tile, uint8_t base_alpha, ItemDrawer *item_drawer,
                       SpriteDrawer *sprite_drawer,
                       CreatureDrawer *creature_drawer) const;
};

#endif
