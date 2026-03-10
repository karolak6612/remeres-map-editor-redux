#ifndef RME_RENDERING_TILE_RENDERER_H_
#define RME_RENDERING_TILE_RENDERER_H_

#include <memory>
#include <stdint.h>

#include "rendering/core/sprite_preload_queue.h"
#include "rendering/drawers/tiles/tile_draw_plan.h"

class TileLocation;
class Tile;
struct DrawContext;
class Editor;
class ItemDrawer;
class SpriteDrawer;
class CreatureDrawer;
class MarkerDrawer;
class SpriteBatch;

struct TileRenderDeps {
    ItemDrawer* item_drawer = nullptr;
    SpriteDrawer* sprite_drawer = nullptr;
    CreatureDrawer* creature_drawer = nullptr;
    MarkerDrawer* marker_drawer = nullptr;
    Editor* editor = nullptr;
};

class TileRenderer {
public:
    explicit TileRenderer(const TileRenderDeps& deps);

    // Convenience wrapper: gathers data then submits GPU commands in one call.
    void DrawTile(
        const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x = -1, int in_draw_y = -1,
        bool draw_lights = false
    );

    // Phase 1: Data gathering. Reads tile data, computes colors, accumulates
    // tooltips/hooks/doors/lights/creature-names into DrawContext accumulators,
    // and builds a list of draw commands in |plan|.
    void PlanTile(
        const DrawContext& ctx, TileLocation* location, uint32_t current_house_id, int in_draw_x, int in_draw_y, bool draw_lights,
        TileDrawPlan& plan
    );

    // Phase 2: GPU submission. Reads the draw commands from |plan| and issues
    // BlitItem, BlitCreature, marker draw, and primitive draw calls.
    // Non-const plan: patterns pointers are fixed up during execution.
    void ExecutePlan(const DrawContext& ctx, TileDrawPlan& plan);

private:
    ItemDrawer* item_drawer;
    SpriteDrawer* sprite_drawer;
    CreatureDrawer* creature_drawer;
    MarkerDrawer* marker_drawer;
    Editor* editor;

    // Reusable plan to avoid per-tile heap allocations.
    // Pre-reserved in constructor for typical tile item counts.
    TileDrawPlan reusable_plan_;

    // Buffered sprite preload requests, flushed after frame submission.
    SpritePreloadQueue preload_queue_;

public:
    // Set the preloader used by the internal SpritePreloadQueue.
    void setPreloader(SpritePreloader* p) { preload_queue_.setPreloader(p); }

    // Flush buffered preload requests. Call after Draw() completes.
    void FlushPreloadQueue()
    {
        if (!preload_queue_.empty()) {
            preload_queue_.processAll();
        }
        preload_queue_.clear();
    }
};

#endif
