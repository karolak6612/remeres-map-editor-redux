#ifndef RME_RENDERING_TILE_RENDERER_H_
#define RME_RENDERING_TILE_RENDERER_H_

#include <memory>
#include <sstream>
#include <stdint.h>

#include "rendering/core/draw_context.h"
#include "rendering/utilities/pattern_calculator.h"

class TileLocation;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class CreatureNameDrawer;
class FloorDrawer;
class MarkerDrawer;
class TooltipCollector;
class GridDrawer;
class SpriteDatabase;
class SpritePreloader;
class LiveManager;

/**
 * @brief Context structure for initializing TileRenderer with necessary drawers and services.
 */
struct TileRenderContext {
    ItemDrawer& item_drawer;
    SpriteDrawer& sprite_drawer;
    CreatureDrawer& creature_drawer;
    CreatureNameDrawer& creature_name_drawer;
    FloorDrawer& floor_drawer;
    MarkerDrawer& marker_drawer;
    TooltipCollector& tooltip_collector;
    GridDrawer& grid_drawer;
    Map& map;
    SpriteDatabase& sprite_database;
    SpritePreloader& sprite_preloader;
    LiveManager* live_manager;
};

class TileRenderer {
public:
    explicit TileRenderer(const TileRenderContext& ctx);

    void DrawTile(
        const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x = -1, int in_draw_y = -1,
        bool draw_lights = false
    );

private:
    void PreloadItem(const Tile* tile, Item* item, const ItemType& it, const SpritePatterns* patterns = nullptr);

    ItemDrawer* item_drawer;
    SpriteDrawer* sprite_drawer;
    CreatureDrawer* creature_drawer;
    FloorDrawer* floor_drawer;
    MarkerDrawer* marker_drawer;
    TooltipCollector* tooltip_collector;
    CreatureNameDrawer* creature_name_drawer;
    Map* map;
    SpriteDatabase* sprite_database;
    SpritePreloader* sprite_preloader;
};

#endif
