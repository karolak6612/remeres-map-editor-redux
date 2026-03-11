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

#include "app/main.h"

#include <algorithm>
#include <type_traits>

#include "app/settings.h"
#include "editor/editor.h"
#include "game/sprites.h"

#include "brushes/brush.h"
#include "brushes/brush_enums.h"
#include "editor/copybuffer.h"
#include "live/live_client.h"
#include "live/live_socket.h"
#include "rendering/core/graphics.h"
#include "rendering/drawers/map_layer_drawer.h"
#include "rendering/map_drawer.h"

#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/editor_map_access.h"
#include "rendering/core/frame_builder.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/brush_visual_settings.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/ui/nvg_image_cache.h"
#include "rendering/ui/tooltip_renderer.h"
#include "rendering/utilities/light_drawer.h"

#include "rendering/core/gl_resources.h"
#include "rendering/core/graphics_sprite_resolver.h"
#include "rendering/core/pending_node_requests.h"
#include "rendering/core/shader_program.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/drawers/cursors/drag_shadow_drawer.h"
#include "rendering/drawers/cursors/live_cursor_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/overlays/preview_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/tiles/shade_drawer.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/io/screen_capture.h"
#include "rendering/postprocess/post_process_manager.h"
#include "rendering/postprocess/post_process_pipeline.h"

MapDrawer::MapDrawer(Editor& editor, RenderContext ctx) :
	editor(editor), map_access_(std::make_unique<EditorMapAccess>(editor)), render_ctx_(ctx), nvg_image_cache(render_ctx_.gfx)
{

    light_drawer = std::make_unique<LightDrawer>();

    // Entity drawers
    entities_.sprite = std::make_unique<SpriteDrawer>();
    entities_.creature = std::make_unique<CreatureDrawer>();
    entities_.item = std::make_unique<ItemDrawer>();
    entities_.marker = std::make_unique<MarkerDrawer>();

    // Orchestrators
    floor_drawer = std::make_unique<FloorDrawer>();

    tile_renderer = std::make_unique<TileRenderer>(TileRenderDeps {
        .item_drawer = entities_.item.get(),
        .sprite_drawer = entities_.sprite.get(),
        .creature_drawer = entities_.creature.get(),
        .marker_drawer = entities_.marker.get(),
        .map_access = map_access_.get()
    });
    // Wire up the preloader so SpritePreloadQueue can call it directly
    // instead of going through the rme::collectTileSprites() indirection.
    tile_renderer->setPreloader(&render_ctx_.gfx.spritePreloader());

    overlays_.grid = std::make_unique<GridDrawer>();
    pending_requests_ = std::make_unique<PendingNodeRequests>();
    pending_requests_->reserve(64);
    map_layer_drawer = std::make_unique<MapLayerDrawer>(tile_renderer.get(), overlays_.grid.get(), map_access_.get(), pending_requests_.get());

    // Cursor drawers
    cursors_.live = std::make_unique<LiveCursorDrawer>();
    cursors_.brush = std::make_unique<BrushCursorDrawer>();
    cursors_.drag_shadow = std::make_unique<DragShadowDrawer>();

    // Overlay drawers
    overlays_.brush_overlay = std::make_unique<BrushOverlayDrawer>();
    overlays_.preview = std::make_unique<PreviewDrawer>();
    overlays_.shade = std::make_unique<ShadeDrawer>();
    overlays_.creature_name = std::make_unique<CreatureNameDrawer>();
    overlays_.hook_indicator = std::make_unique<HookIndicatorDrawer>();
    overlays_.door_indicator = std::make_unique<DoorIndicatorDrawer>();

    // Infrastructure
    sprite_batch = std::make_unique<SpriteBatch>();
    primitive_renderer = std::make_unique<PrimitiveRenderer>();
    post_process_ = std::make_unique<PostProcessPipeline>();

    sprite_resolver = std::make_unique<GraphicsSpriteResolver>(render_ctx_.gfx);
    entities_.item->SetSpriteResolver(sprite_resolver.get());
    entities_.creature->SetSpriteResolver(sprite_resolver.get());
    entities_.sprite->SetSpriteResolver(sprite_resolver.get());

    // Pre-reserve both accumulator buffers and light buffer for typical frame sizes
    accumulators_[0].reserve(256, 128, 64);
    accumulators_[1].reserve(256, 128, 64);
    frames_[0].lights.reserve(512);
    frames_[1].lights.reserve(512);
}

MapDrawer::~MapDrawer()
{

    Release();
}

void MapDrawer::SetupVars(DrawFrame frame)
{
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    writeFrame() = std::move(frame);
    render_frame_index_ = write_frame_index_.load(std::memory_order_relaxed);
    frame_ready_.store(true, std::memory_order_release);
}

void MapDrawer::SetupVars(
    const ViewSnapshot& snapshot, const BrushSnapshot& brush, const BrushVisualSettings& brush_visual, const RenderSettings& settings,
    const FrameOptions& base_options
)
{
    SetupVars(FrameBuilder::Build(snapshot, brush, brush_visual, settings, base_options, editor, render_ctx_.gfx.getAtlasManager()));
}

void MapDrawer::SetupGL()
{
    auto& frame = renderFrame();
    GLViewport::Apply(frame.view);
    ViewProjection::Compute(frame.view);

    // Ensure renderers are initialized
    if (!renderers_initialized_) {
        const bool sprite_batch_initialized = sprite_batch->initialize(render_ctx_.gfx.sharedGeometry());
        if (!sprite_batch_initialized) {
            return;
        }
        primitive_renderer->initialize();
        renderers_initialized_ = true;
    }
}

void MapDrawer::Release() { }

void MapDrawer::Draw()
{
    if (!frame_ready_.load(std::memory_order_acquire)) {
        return;
    }

    auto& frame = renderFrame();
    render_ctx_.gfx.updateTime();

    frame.lights.Clear();
    frame.options.transient_selection_bounds = std::nullopt;

    if (frame.options.boundbox_selection) {
        frame.options.transient_selection_bounds = MapBounds {
            .x1 = std::min(frame.snapshot.last_click_map_x, frame.snapshot.last_cursor_map_x),
            .y1 = std::min(frame.snapshot.last_click_map_y, frame.snapshot.last_cursor_map_y),
            .x2 = std::max(frame.snapshot.last_click_map_x, frame.snapshot.last_cursor_map_x),
            .y2 = std::max(frame.snapshot.last_click_map_y, frame.snapshot.last_cursor_map_y)
        };
    }

    if (!render_ctx_.gfx.ensureAtlasManager()) {
        return;
    }
    frame.atlas = render_ctx_.gfx.getAtlasManager();

    // Begin Batches
    sprite_batch->begin(frame.view.projectionMatrix, *frame.atlas);
    primitive_renderer->setProjectionMatrix(frame.view.projectionMatrix);

    // Post-processing: bind FBO if shader or AA is active
    bool use_fbo = post_process_->Begin(frame.view, frame.settings);

    GLViewport::Clear(); // Clear screen (or FBO)

    // Construct the frame DrawContext once — all private methods receive it by const ref.
    const DrawContext ctx {*sprite_batch, *primitive_renderer, frame.view, frame.settings, frame.options, frame.lights, writeAccumulators(), *frame.atlas};

    DrawMap(ctx);

    // Flush Map for Light Pass
    sprite_batch->end(*frame.atlas);
    primitive_renderer->flush();

    if (frame.settings.isDrawLight()) {
        DrawLight();
    }

    // If using FBO, resolve to screen
    if (use_fbo) {
        post_process_->End(frame.view, frame.settings);
    }

    // Resume Batch for Overlays
    sprite_batch->begin(frame.view.projectionMatrix, *frame.atlas);

    if (cursors_.drag_shadow) {
        cursors_.drag_shadow->draw(
            ctx, *map_access_, entities_.item.get(), entities_.sprite.get(), entities_.creature.get(), frame.snapshot.drag_start
        );
    }

    cursors_.live->draw(ctx, *map_access_);

    {
        const BrushOverlayContext overlay {
            .item_drawer = entities_.item.get(),
            .sprite_drawer = entities_.sprite.get(),
            .creature_drawer = entities_.creature.get(),
            .brush_cursor_drawer = cursors_.brush.get(),
            .map_access = map_access_.get(),
            .visual = &frame.brush_visual,
            .current_brush = frame.brush.current_brush,
            .brush_shape = frame.brush.brush_shape,
            .brush_size = frame.brush.brush_size,
            .is_drawing_mode = frame.brush.is_drawing_mode,
            .is_dragging_draw = frame.snapshot.is_dragging_draw,
            .last_click_map_x = frame.snapshot.last_click_map_x,
            .last_click_map_y = frame.snapshot.last_click_map_y
        };
        overlays_.brush_overlay->draw(ctx, overlay);
    }

    const ViewBounds base_bounds {frame.view.start_x, frame.view.start_y, frame.view.end_x, frame.view.end_y};

    if (frame.settings.show_grid) {
        DrawGrid(ctx, base_bounds);
    }
    if (frame.settings.show_ingame_box) {
        DrawIngameBox(ctx, base_bounds);
    }

    // Draw creature names (Overlay) moved to DrawCreatureNames()

    // End Batches and Flush
    sprite_batch->end(*frame.atlas);
    primitive_renderer->flush();

    // Flush buffered sprite preload requests after GPU submission
    tile_renderer->FlushPreloadQueue();

    // Tooltips are now drawn in MapCanvas::OnPaint (UI Pass)
}

void MapDrawer::DrainPendingNodeRequests()
{
    auto requests = pending_requests_->drain();
    for (const auto& req : requests) {
        if (editor.live_manager.GetClient()) {
            editor.live_manager.GetClient()->queryNode(req.x, req.y, req.underground);
        }
    }
}

void MapDrawer::DrawMap(const DrawContext& ctx)
{
    const auto& frame = renderFrame();
    bool live_client = editor.live_manager.IsClient();

    int floor_offset = 0;

    for (int map_z = frame.view.start_z; map_z >= frame.view.superend_z; map_z--) {
        FloorViewParams floor_params {
            frame.view.start_x - floor_offset, frame.view.start_y - floor_offset, frame.view.end_x + floor_offset, frame.view.end_y + floor_offset
        };

        if (map_z == frame.view.end_z && frame.view.start_z != frame.view.end_z) {
            overlays_.shade->draw(ctx);
        }

        if (map_z >= frame.view.end_z) {
            DrawMapLayer(ctx, map_z, live_client, floor_params);
        }

        overlays_.preview->draw(ctx, PreviewDrawerContext {
            .snapshot = frame.snapshot,
            .floor_params = floor_params,
            .map_z = map_z,
            .map_access = *map_access_,
            .item_drawer = entities_.item.get(),
            .sprite_drawer = entities_.sprite.get(),
            .creature_drawer = entities_.creature.get(),
            .current_brush = frame.brush.current_brush,
        });

        ++floor_offset;
    }
}

void MapDrawer::DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds)
{
    overlays_.grid->DrawIngameBox(ctx, bounds);
}

void MapDrawer::DrawGrid(const DrawContext& ctx, const ViewBounds& bounds)
{
    overlays_.grid->DrawGrid(ctx, bounds);
}

void MapDrawer::DrawTooltips(NVGcontext* vg)
{
    tooltip_renderer.draw(vg, renderFrame().view, readAccumulators().tooltips.getTooltips(), nvg_image_cache);
}

void MapDrawer::DrawHookIndicators(NVGcontext* vg)
{
    overlays_.hook_indicator->draw(vg, renderFrame().view, readAccumulators().hooks);
}

void MapDrawer::DrawDoorIndicators(NVGcontext* vg)
{
    if (renderFrame().settings.highlight_locked_doors) {
        overlays_.door_indicator->draw(vg, renderFrame().view, readAccumulators().doors);
    }
}

void MapDrawer::DrawCreatureNames(NVGcontext* vg)
{
    overlays_.creature_name->draw(vg, renderFrame().view, readAccumulators().creature_names);
}

void MapDrawer::DrawMapLayer(const DrawContext& ctx, int map_z, bool live_client, const FloorViewParams& floor_params)
{
    map_layer_drawer->Draw(ctx, map_z, live_client, floor_params, draw_command_queue_);
    SubmitDrawCommands(ctx, draw_command_queue_);
}

void MapDrawer::SubmitDrawCommands(const DrawContext& ctx, const DrawCommandQueue& queue)
{
    for (const auto& command : queue.commands()) {
        std::visit(
            [&](const auto& cmd) {
                using Command = std::decay_t<decltype(cmd)>;

                if constexpr (std::is_same_v<Command, DrawColorSquareCmd>) {
                    entities_.sprite->glBlitSquare(ctx.sprite_batch, ctx.atlas, cmd.draw_x, cmd.draw_y, cmd.color);
                } else if constexpr (std::is_same_v<Command, DrawZoneBrushCmd>) {
                    entities_.item->DrawRawBrush(
                        ctx.sprite_batch, entities_.sprite.get(), cmd.draw_x, cmd.draw_y, cmd.sprite_id, cmd.r, cmd.g, cmd.b, cmd.a
                    );
                } else if constexpr (std::is_same_v<Command, DrawHouseBorderCmd>) {
                    entities_.sprite->glDrawBox(ctx.sprite_batch, ctx.atlas, cmd.draw_x, cmd.draw_y, 32, 32, cmd.color);
                } else if constexpr (std::is_same_v<Command, DrawItemCmd>) {
                    auto params = cmd.params;
                    auto patterns = cmd.patterns;
                    params.patterns = &patterns;
                    int draw_x = cmd.draw_x;
                    int draw_y = cmd.draw_y;
                    entities_.item->BlitItem(
                        ctx.sprite_batch, ctx.atlas, entities_.sprite.get(), entities_.creature.get(), draw_x, draw_y, params
                    );
                } else if constexpr (std::is_same_v<Command, DrawCreatureCmd>) {
                    entities_.creature->BlitCreature(
                        ctx.sprite_batch, entities_.sprite.get(), cmd.draw_x, cmd.draw_y, cmd.creature, cmd.options
                    );
                } else if constexpr (std::is_same_v<Command, DrawMarkerCmd>) {
                    entities_.marker->draw(
                        ctx.sprite_batch, entities_.sprite.get(), cmd.draw_x, cmd.draw_y, cmd.tile, cmd.waypoint, cmd.current_house_id,
                        *map_access_, ctx.settings
                    );
                }
            },
            command
        );
    }
}

void MapDrawer::DrawLight()
{
    light_drawer->draw(
        renderFrame().view, renderFrame().settings.experimental_fog, renderFrame().lights, renderFrame().options.global_light_color,
        renderFrame().settings.light_intensity, renderFrame().settings.ambient_light_level
    );
}

void MapDrawer::TakeScreenshot(uint8_t* screenshot_buffer)
{
    ScreenCapture::Capture(renderFrame().view.screensize_x, renderFrame().view.screensize_y, screenshot_buffer);
}

void MapDrawer::BeginFrame()
{
    // Swap: current write buffer becomes the read buffer for overlay drawing,
    // then clear the new write buffer for the next frame.
    write_index_ = 1 - write_index_;
    writeAccumulators().clear();
    write_frame_index_.store(1 - render_frame_index_, std::memory_order_release);
    frame_ready_.store(false, std::memory_order_release);
}
