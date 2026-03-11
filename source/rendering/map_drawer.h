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
#include "rendering/core/draw_frame.h"
#include "rendering/core/gl_resources.h"
#include "rendering/core/prepared_frame_buffer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_prep_snapshot.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/ui/nvg_image_cache.h"
#include "rendering/ui/tooltip_renderer.h"

#include <atomic>
#include <mutex>

struct DrawContext;
struct ViewBounds;
struct FloorViewParams;
class PostProcessPipeline;
class AtlasManager;
class GridDrawer;
class PendingNodeRequests;
class EditorMapAccess;
class TilePlanningPool;

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
    std::unique_ptr<EditorMapAccess> map_access_;
    RenderContext render_ctx_;
    PreparedFrameBuffer prepared_frames_[2];
    std::atomic<int> write_prepared_index_ {0};
    int render_prepared_index_ = 0;
    std::atomic<bool> has_prepared_frame_ {false};
    mutable std::mutex snapshot_mutex_; // Protects frame snapshots for future multi-threaded access
    std::unique_ptr<LightDrawer> light_drawer;

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

public:
    MapDrawer(Editor& editor, RenderContext ctx);
    ~MapDrawer();

    [[nodiscard]] RenderPrepSnapshot BuildRenderPrepSnapshot(
        const ViewSnapshot& snapshot, const BrushSnapshot& brush, const BrushVisualSettings& brush_visual, const RenderSettings& settings,
        const FrameOptions& base_options
    );
    [[nodiscard]] PreparedFrameBuffer PrepareFrame(RenderPrepSnapshot snapshot);
    void SetupPreparedFrame(PreparedFrameBuffer prepared);
    void SetPlanningPool(TilePlanningPool* planning_pool);
    [[nodiscard]] bool HasPreparedFrame() const
    {
        return has_prepared_frame_.load(std::memory_order_acquire);
    }
    void SetupVars(
        const ViewSnapshot& snapshot, const BrushSnapshot& brush, const BrushVisualSettings& brush_visual, const RenderSettings& settings,
        const FrameOptions& base_options
    );
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
    void DrainPendingNodeRequests();
    void DrawCreatureNames(NVGcontext* vg);

    void DrawLight();

    void TakeScreenshot(uint8_t* screenshot_buffer);

    RenderSettings& getRenderSettings()
    {
        return renderFrame().settings;
    }
    FrameOptions& getFrameOptions()
    {
        return renderFrame().options;
    }
    ViewSnapshot getSnapshot() const
    {
        std::lock_guard<std::mutex> lock(snapshot_mutex_);
        return renderFrame().snapshot;
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
    void DrawMapLayer(const DrawContext& ctx, int map_z);
    void SubmitDrawCommands(const DrawContext& ctx, const DrawCommandQueue& queue, size_t command_start, size_t command_count);
    void DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds);
    void DrawGrid(const DrawContext& ctx, const ViewBounds& bounds);
    PreparedFrameBuffer& writePreparedFrame()
    {
        return prepared_frames_[write_prepared_index_.load(std::memory_order_relaxed)];
    }
    PreparedFrameBuffer& renderPreparedFrame()
    {
        return prepared_frames_[render_prepared_index_];
    }
    const PreparedFrameBuffer& renderPreparedFrame() const
    {
        return prepared_frames_[render_prepared_index_];
    }
    DrawFrame& renderFrame()
    {
        return renderPreparedFrame().frame;
    }
    const DrawFrame& renderFrame() const
    {
        return renderPreparedFrame().frame;
    }
    const FrameAccumulators& readAccumulators() const
    {
        return renderPreparedFrame().accumulators;
    }
    bool renderers_initialized_ = false;
};

#endif
