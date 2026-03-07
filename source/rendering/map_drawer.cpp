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

#include "editor/editor.h"
#include "game/sprites.h"
#include "ui/gui.h"

#include "brushes/brush.h"
#include "editor/copybuffer.h"
#include "live/live_socket.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/gl_viewport.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/texture_gc.h"
#include "rendering/drawers/map_layer_drawer.h"
#include "rendering/io/sprite_loader.h"
#include "rendering/map_drawer.h"
#include "rendering/ui/drawing_controller.h"
#include "rendering/ui/map_display.h"
#include "rendering/ui/selection_controller.h"
#include <glm/gtc/matrix_transform.hpp>

#include "brushes/carpet/carpet_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/view_state.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/utilities/light_drawer.h"

#include "rendering/core/gl_resources.h"
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

MapDrawer::MapDrawer(MapCanvas &canvas, Editor &editor)
    : canvas(canvas), editor(editor) {

  light_drawer = std::make_shared<LightDrawer>();
  tooltip_drawer = std::make_unique<TooltipDrawer>();

  sprite_drawer = std::make_unique<SpriteDrawer>();
  creature_drawer = std::make_unique<CreatureDrawer>();
  floor_drawer = std::make_unique<FloorDrawer>();
  item_drawer = std::make_unique<ItemDrawer>();
  marker_drawer = std::make_unique<MarkerDrawer>();

  creature_name_drawer = std::make_unique<CreatureNameDrawer>();

  tile_renderer = std::make_unique<TileRenderer>(TileRenderContext{
      .item_drawer = *item_drawer,
      .sprite_drawer = *sprite_drawer,
      .creature_drawer = *creature_drawer,
      .creature_name_drawer = *creature_name_drawer,
      .floor_drawer = *floor_drawer,
      .marker_drawer = *marker_drawer,
      .tooltip_drawer = *tooltip_drawer,
      .editor = editor});

  grid_drawer = std::make_unique<GridDrawer>();
  map_layer_drawer =
      std::make_unique<MapLayerDrawer>(tile_renderer.get(), grid_drawer.get(),
                                       &editor); // Initialized map_layer_drawer
  live_cursor_drawer = std::make_unique<LiveCursorDrawer>();
  brush_cursor_drawer = std::make_unique<BrushCursorDrawer>();
  brush_overlay_drawer = std::make_unique<BrushOverlayDrawer>();
  drag_shadow_drawer = std::make_unique<DragShadowDrawer>();
  preview_drawer = std::make_unique<PreviewDrawer>();

  shade_drawer = std::make_unique<ShadeDrawer>();

  sprite_batch = std::make_unique<SpriteBatch>();
  primitive_renderer = std::make_unique<PrimitiveRenderer>();
  hook_indicator_drawer = std::make_unique<HookIndicatorDrawer>();
  door_indicator_drawer = std::make_unique<DoorIndicatorDrawer>();

  item_drawer->SetHookIndicatorDrawer(hook_indicator_drawer.get());
  item_drawer->SetDoorIndicatorDrawer(door_indicator_drawer.get());

  post_process_pipeline =
      std::make_unique<PostProcessPipeline>(post_process_manager_);
}

MapDrawer::~MapDrawer() {}

void MapDrawer::SetupDrawingOptions() {
  options.frame.current_house_id = 0;
  Brush *brush = g_gui.GetCurrentBrush();
  if (brush) {
    if (brush->is<HouseBrush>()) {
      options.frame.current_house_id = brush->as<HouseBrush>()->getHouseID();
    } else if (brush->is<HouseExitBrush>()) {
      options.frame.current_house_id =
          brush->as<HouseExitBrush>()->getHouseID();
    }
  }

  // Calculate pulse for house highlighting
  // Period is 1 second (1000ms)
  // Range is [0.0, 1.0]
  // Using a sine wave for smooth transition
  // (sin(t) + 1) / 2
  double now = wxGetLocalTimeMillis().ToDouble();
  const double speed = 0.005;
  options.frame.highlight_pulse =
      static_cast<float>((sin(now * speed) + 1.0) / 2.0);
}

void MapDrawer::SetupViewState() {
  // Setup ViewState from canvas
  view.zoom = static_cast<float>(canvas.GetZoom());
  view.floor = canvas.GetFloor();
  canvas.GetViewBox(&view.view_scroll_x, &view.view_scroll_y,
                    &view.screensize_x, &view.screensize_y);

  view.viewport_x = 0;
  view.viewport_y = 0;

  view.mouse_map_x = canvas.last_cursor_map_x;
  view.mouse_map_y = canvas.last_cursor_map_y;

  view.tile_size = std::max(1, static_cast<int>(TILE_SIZE / view.zoom));
  view.camera_pos.z = view.floor;

  // Calculate bounds based on options and map layers
  if (options.settings.show_all_floors) {
    if (view.floor <= GROUND_LAYER) {
      view.start_z = GROUND_LAYER;
    } else {
      view.start_z = std::min(MAP_MAX_LAYER, view.floor + 2);
    }
  } else {
    view.start_z = view.floor;
  }

  view.end_z = view.floor;
  view.superend_z = (view.floor > GROUND_LAYER ? 8 : 0);

  if (options.settings.show_shade) {
    if (view.end_z < view.start_z && view.end_z == view.superend_z &&
        view.end_z >= 0) {
      view.superend_z--;
    }
    if (view.superend_z < 0) {
      view.superend_z = 0;
    }
  }

  view.camera_start_x = view.view_scroll_x / TILE_SIZE;
  view.camera_start_y = view.view_scroll_y / TILE_SIZE;

  if (view.floor > GROUND_LAYER) {
    view.camera_start_x -= 2;
    view.camera_start_y -= 2;
  }

  view.camera_end_x =
      view.camera_start_x + view.screensize_x / view.tile_size + 2;
  view.camera_end_y =
      view.camera_start_y + view.screensize_y / view.tile_size + 2;

  // Calculate logical dimensions (matching RenderView behavior)
  view.logical_width = view.screensize_x * view.zoom;
  view.logical_height = view.screensize_y * view.zoom;

  // Metrics update for OpenGL transformations
  view.projectionMatrix = glm::ortho(
      0.0f, static_cast<float>(view.screensize_x) * view.zoom,
      static_cast<float>(view.screensize_y) * view.zoom, 0.0f, -1.0f, 1.0f);
  view.viewMatrix =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.375f, 0.375f, 0.0f));
}

void MapDrawer::SetupCanvasState() {
  // Populate canvas state for DrawContext
  canvas_state.last_click_map_x = canvas.last_click_map_x;
  canvas_state.last_click_map_y = canvas.last_click_map_y;
  canvas_state.last_click_map_z = canvas.last_click_map_z;
  canvas_state.last_click_abs_x = canvas.last_click_abs_x;
  canvas_state.last_click_abs_y = canvas.last_click_abs_y;
  canvas_state.cursor_x = canvas.cursor_x;
  canvas_state.cursor_y = canvas.cursor_y;
  canvas_state.is_dragging_draw =
      canvas.drawing_controller && canvas.drawing_controller->IsDraggingDraw();
  canvas_state.is_pasting = canvas.isPasting();
  canvas_state.drag_start_pos =
      canvas.selection_controller
          ? canvas.selection_controller->GetDragStartPosition()
          : Position();

  MapTab *mapTab = dynamic_cast<MapTab *>(canvas.GetMapWindow());
  canvas_state.secondary_map =
      mapTab ? mapTab->GetSession()->secondary_map : nullptr;
  canvas_state.current_house_id = options.frame.current_house_id;
}

void MapDrawer::SetupGL() {
  GLViewport::Apply(view);

  // Ensure renderers are initialized
  if (!renderers_initialized) {

    sprite_batch->initialize();
    primitive_renderer->initialize();
    renderers_initialized = true;
  }

  post_process_pipeline->Initialize();
}

void MapDrawer::Draw() {
  // Redundant update removed: MapCanvas::OnPaint advances timing BEFORE calling
  // drawer->Draw() g_gui.gc.updateTime();

  light_buffer.Clear();
  creature_name_drawer->clear();
  options.frame.transient_selection_bounds = std::nullopt;

  if (options.frame.boundbox_selection) {
    options.frame.transient_selection_bounds = MapBounds{
        .x1 = std::min(canvas.last_click_map_x, canvas.last_cursor_map_x),
        .y1 = std::min(canvas.last_click_map_y, canvas.last_cursor_map_y),
        .x2 = std::max(canvas.last_click_map_x, canvas.last_cursor_map_x),
        .y2 = std::max(canvas.last_click_map_y, canvas.last_cursor_map_y)};
  }

  if (!g_gui.atlas.ensureAtlasManager()) {
    return;
  }
  auto *atlas = g_gui.atlas.getAtlasManager();

  // Begin Batches
  sprite_batch->begin(view.projectionMatrix, *atlas);
  primitive_renderer->setProjectionMatrix(view.projectionMatrix);

  // Check Framebuffer Logic
  bool use_fbo = post_process_pipeline->BeginCapture(view, options);

  DrawBackground(); // Clear screen (or FBO)

  // Save original view bounds before DrawMap modifies them per-floor
  const ViewBounds original_bounds{view.camera_start_x, view.camera_start_y,
                                   view.camera_end_x, view.camera_end_y};

  DrawMap();

  // Flush Map for Light Pass
  sprite_batch->end(*atlas);
  primitive_renderer->flush();

  if (options.settings.isDrawLight()) {
    DrawLight();
  }

  // If using FBO, we must now Resolve to Screen
  if (use_fbo) {
    post_process_pipeline->EndCaptureAndDraw(view, options);
  }

  // Resume Batch for Overlays
  sprite_batch->begin(view.projectionMatrix, *atlas);

  DrawContext ctx = MakeDrawContext();

  if (drag_shadow_drawer) {
    drag_shadow_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(),
                             creature_drawer.get(), editor);
  }

  live_cursor_drawer->draw(ctx, editor);

  brush_overlay_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(),
                             creature_drawer.get(), editor);

  if (options.settings.show_grid) {
    DrawGrid(ctx, original_bounds);
  }
  if (options.settings.show_ingame_box) {
    DrawIngameBox(ctx, original_bounds);
  }

  // Draw creature names (Overlay) moved to DrawCreatureNames()

  // End Batches and Flush
  sprite_batch->end(*atlas);
  primitive_renderer->flush();

  // Tooltips are now drawn in MapCanvas::OnPaint (UI Pass)
}

void MapDrawer::DrawBackground() {
  GLViewport::ClearBackground(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), true);
}

void MapDrawer::DrawMap() {
  bool live_client = editor.live_manager.IsClient();

  bool only_colors =
      options.settings.show_as_minimap || options.settings.show_only_colors;

  // Enable texture mode

  int current_start_x = view.camera_start_x;
  int current_start_y = view.camera_start_y;
  int current_end_x = view.camera_end_x;
  int current_end_y = view.camera_end_y;

  DrawContext ctx = MakeDrawContext();

  for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
    FloorViewParams floor_params{.current_z = map_z,
                                 .start_x = current_start_x,
                                 .start_y = current_start_y,
                                 .end_x = current_end_x,
                                 .end_y = current_end_y};

    if (map_z == view.end_z && view.start_z != view.end_z) {
      shade_drawer->draw(ctx, floor_params);
    }

    if (map_z >= view.end_z) {
      DrawMapLayer(ctx, floor_params, live_client);
    }

    preview_drawer->draw(ctx, floor_params, map_z, editor, item_drawer.get(),
                         sprite_drawer.get(), creature_drawer.get());

    --current_start_x;
    --current_start_y;
    ++current_end_x;
    ++current_end_y;
  }
}

void MapDrawer::DrawIngameBox(const DrawContext &ctx,
                              const ViewBounds &bounds) {
  grid_drawer->DrawIngameBox(ctx, bounds);
}

void MapDrawer::DrawGrid(const DrawContext &ctx, const ViewBounds &bounds) {
  grid_drawer->DrawGrid(ctx, bounds);
}

void MapDrawer::DrawTooltips(NVGcontext *vg) {
  if (options.settings.show_tooltips) {
    DrawContext ctx = MakeDrawContext();
    tooltip_drawer->draw(vg, ctx);
  }
}

void MapDrawer::DrawHookIndicators(NVGcontext *vg) {
  DrawContext ctx = MakeDrawContext();
  hook_indicator_drawer->draw(vg, ctx);
}

void MapDrawer::DrawDoorIndicators(NVGcontext *vg) {
  if (options.settings.highlight_locked_doors) {
    DrawContext ctx = MakeDrawContext();
    door_indicator_drawer->draw(vg, ctx);
  }
}

void MapDrawer::DrawCreatureNames(NVGcontext *vg) {
  DrawContext ctx = MakeDrawContext();
  creature_name_drawer->draw(vg, ctx);
}

void MapDrawer::DrawMapLayer(const DrawContext &ctx,
                             const FloorViewParams &floor_params,
                             bool live_client) {
  map_layer_drawer->Draw(ctx, floor_params, live_client);
}

DrawContext MapDrawer::MakeDrawContext() {
  return DrawContext{.sprite_batch = *sprite_batch,
                     .primitive_renderer = *primitive_renderer,
                     .view = view,
                     .options = options,
                     .light_buffer = light_buffer,
                     .canvas_state = canvas_state,
                     .brush_cursor_drawer = brush_cursor_drawer.get()};
}

void MapDrawer::DrawLight() {
  if (options.settings.isDrawLight()) {
    DrawContext ctx = MakeDrawContext();
    light_drawer->draw(ctx);
  }
}

void MapDrawer::TakeScreenshot(uint8_t *screenshot_buffer) {
  ScreenCapture::Capture(view.screensize_x, view.screensize_y,
                         screenshot_buffer);
}

void MapDrawer::ClearFrameOverlays() {
  tooltip_drawer->clear();
  hook_indicator_drawer->clear();
  door_indicator_drawer->clear();
}
