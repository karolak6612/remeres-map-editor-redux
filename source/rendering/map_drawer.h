//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_MAP_DRAWER_H_
#define RME_MAP_DRAWER_H_
#include <memory>
class GameSprite;

struct NVGcontext;

// Storage during drawing, for option caching
#include "rendering/core/render_context.h"
#include "app/definitions.h"
#include "game/creature.h"
#include "game/outfit.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/render_settings.h"

#include "rendering/core/frame_accumulators.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/brush_snapshot.h"
#include "rendering/core/view_snapshot.h"
#include "rendering/ui/nvg_image_cache.h"
#include "rendering/ui/tooltip_renderer.h"

#include <mutex>

struct DrawContext;
struct ViewBounds;
struct FloorViewParams;
class PostProcessPipeline;
class AtlasManager;
class GridDrawer;
class PendingNodeRequests;

class LightDrawer;
class GraphicsSpriteResolver;
class LiveCursorDrawer;
class BrushCursorDrawer;
class BrushOverlayDrawer;
class DragShadowDrawer;
class FloorDrawer;
class SpriteDrawer;
class ItemDrawer;
class MapLayerDrawer;
class CreatureDrawer;
class MarkerDrawer;
class PreviewDrawer;
class ShadeDrawer;
class TileRenderer;
class CreatureNameDrawer;
class HookIndicatorDrawer;
class DoorIndicatorDrawer;

// Logical groupings for MapDrawer's many sub-drawers.
// Each group owns related drawers via unique_ptr.

struct EntityDrawers {
    std::unique_ptr<SpriteDrawer> sprite;
    std::unique_ptr<ItemDrawer> item;
    std::unique_ptr<CreatureDrawer> creature;
    std::unique_ptr<MarkerDrawer> marker;
};

struct OverlayDrawers {
    std::unique_ptr<GridDrawer> grid;
    std::unique_ptr<HookIndicatorDrawer> hook_indicator;
    std::unique_ptr<DoorIndicatorDrawer> door_indicator;
    std::unique_ptr<PreviewDrawer> preview;
    std::unique_ptr<ShadeDrawer> shade;
    std::unique_ptr<BrushOverlayDrawer> brush_overlay;
    std::unique_ptr<CreatureNameDrawer> creature_name;
};

struct CursorDrawers {
    std::unique_ptr<LiveCursorDrawer> live;
    std::unique_ptr<BrushCursorDrawer> brush;
    std::unique_ptr<DragShadowDrawer> drag_shadow;
};

class MapDrawer {
    Editor& editor;
    RenderContext render_ctx_;
    RenderSettings render_settings;
    FrameOptions frame_options;
    ViewState view;
    ViewSnapshot snapshot_;
    BrushSnapshot brush_snapshot_;
    mutable std::mutex snapshot_mutex_; // Protects snapshot_ for future multi-threaded access
    std::unique_ptr<LightDrawer> light_drawer;
    LightBuffer light_buffer;

    // Double-buffered accumulators: write buffer accumulates during Draw(),
    // read buffer is consumed by overlay drawers (tooltips, hooks, doors, names).
    // Swapped at frame boundary in BeginFrame().
    FrameAccumulators accumulators_[2];
    int write_index_ = 0;
    FrameAccumulators& writeAccumulators()
    {
        return accumulators_[write_index_];
    }
    const FrameAccumulators& readAccumulators() const
    {
        return accumulators_[1 - write_index_];
    }

    TooltipRenderer tooltip_renderer;
    NVGImageCache nvg_image_cache;

    // Grouped sub-drawers
    EntityDrawers entities_;
    OverlayDrawers overlays_;
    CursorDrawers cursors_;

    // Orchestrators (not categorized — they coordinate multiple drawers)
    std::unique_ptr<FloorDrawer> floor_drawer;
    std::unique_ptr<MapLayerDrawer> map_layer_drawer;
    std::unique_ptr<TileRenderer> tile_renderer;

    // Infrastructure
    std::unique_ptr<GraphicsSpriteResolver> sprite_resolver;
    std::unique_ptr<SpriteBatch> sprite_batch;
    std::unique_ptr<PrimitiveRenderer> primitive_renderer;

    // Deferred network node requests — filled during Draw(), drained after
    std::unique_ptr<PendingNodeRequests> pending_requests_;

    // Post-processing
    std::unique_ptr<PostProcessPipeline> post_process_;

    // Cached per-frame atlas pointer (set in Draw(), valid until frame ends)
    AtlasManager* current_atlas_ = nullptr;

public:
    MapDrawer(Editor& editor, RenderContext ctx);
    ~MapDrawer();

    void SetupVars(const ViewSnapshot& snapshot, const BrushSnapshot& brush);
    void SetupGL();
    void Release();

    void Draw();
    void DrawLiveCursors();
    void DrawTooltips(NVGcontext* vg);
    void DrawHookIndicators(NVGcontext* vg);
    void DrawDoorIndicators(NVGcontext* vg);
    // Swap accumulator buffers and clear the new write buffer.
    // Call once at the start of each frame, before Draw().
    void BeginFrame();
    void DrawCreatureNames(NVGcontext* vg);

    void DrawLight();

    void TakeScreenshot(uint8_t* screenshot_buffer);

    RenderSettings& getRenderSettings()
    {
        return render_settings;
    }
    FrameOptions& getFrameOptions()
    {
        return frame_options;
    }
    ViewSnapshot getSnapshot() const
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        return snapshot_;
    }

    SpriteBatch* getSpriteBatch()
    {
        return sprite_batch.get();
    }
    PrimitiveRenderer* getPrimitiveRenderer()
    {
        return primitive_renderer.get();
    }
    TileRenderer* getTileRenderer()
    {
        return tile_renderer.get();
    }

private:
    void DrawMap(const DrawContext& ctx);
    void DrawMapLayer(const DrawContext& ctx, int map_z, bool live_client, const FloorViewParams& floor_params);
    void DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds);
    void DrawGrid(const DrawContext& ctx, const ViewBounds& bounds);
    bool renderers_initialized = false;
};

#endif
