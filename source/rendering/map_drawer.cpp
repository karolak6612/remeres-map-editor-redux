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
#include "ui/gui.h"
#include "game/sprites.h"

#include "rendering/map_drawer.h"
#include "brushes/brush.h"
#include "rendering/drawers/map_layer_drawer.h"
#include "rendering/ui/map_display.h"
#include "editor/copybuffer.h"
#include "live/live_socket.h"
#include "rendering/core/graphics.h"

#include "brushes/doodad/doodad_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/waypoint/waypoint_brush.h"
#include "rendering/utilities/light_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"

#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/cursors/live_cursor_drawer.h"
#include "rendering/drawers/overlays/selection_drawer.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"
#include "rendering/drawers/cursors/drag_shadow_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/preview_drawer.h"
#include "rendering/drawers/tiles/shade_drawer.h"
#include "rendering/drawers/tiles/tile_color_calculator.h"
#include "rendering/io/screen_capture.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/core/gl_resources.h"
#include "rendering/postprocess/post_process_pipeline.h"
#include "rendering/postprocess/post_process_manager.h"
#include "rendering/core/draw_context.h"
#include "rendering/ui/drawing_controller.h"
#include "rendering/ui/selection_controller.h"

// Inline fallback for screen vertex shader (used when shaders/ dir is missing)
static constexpr const char* kScreenVertFallback = R"(
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

MapDrawer::MapDrawer(MapCanvas* canvas) :
	canvas(canvas), editor(canvas->editor) {

	light_drawer = std::make_shared<LightDrawer>();
	tooltip_drawer = std::make_unique<TooltipDrawer>();

	sprite_drawer = std::make_unique<SpriteDrawer>();
	creature_drawer = std::make_unique<CreatureDrawer>();
	floor_drawer = std::make_unique<FloorDrawer>();
	item_drawer = std::make_unique<ItemDrawer>();
	marker_drawer = std::make_unique<MarkerDrawer>();

	creature_name_drawer = std::make_unique<CreatureNameDrawer>();

	tile_renderer = std::make_unique<TileRenderer>(item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), creature_name_drawer.get(), floor_drawer.get(), marker_drawer.get(), tooltip_drawer.get(), &editor);

	grid_drawer = std::make_unique<GridDrawer>();
	map_layer_drawer = std::make_unique<MapLayerDrawer>(tile_renderer.get(), grid_drawer.get(), &editor); // Initialized map_layer_drawer
	live_cursor_drawer = std::make_unique<LiveCursorDrawer>();
	selection_drawer = std::make_unique<SelectionDrawer>();
	brush_cursor_drawer = std::make_unique<BrushCursorDrawer>();
	brush_overlay_drawer = std::make_unique<BrushOverlayDrawer>();
	drag_shadow_drawer = std::make_unique<DragShadowDrawer>();
	preview_drawer = std::make_unique<PreviewDrawer>();

	shade_drawer = std::make_unique<ShadeDrawer>();

	sprite_batch = std::make_unique<SpriteBatch>();
	primitive_renderer = std::make_unique<PrimitiveRenderer>();
	post_process_pipeline = std::make_unique<PostProcessPipeline>(PostProcessManager::Instance());
	hook_indicator_drawer = std::make_unique<HookIndicatorDrawer>();
	door_indicator_drawer = std::make_unique<DoorIndicatorDrawer>();

	item_drawer->SetHookIndicatorDrawer(hook_indicator_drawer.get());
	item_drawer->SetDoorIndicatorDrawer(door_indicator_drawer.get());
}

MapDrawer::~MapDrawer() {

	Release();
}

void MapDrawer::SetupVars() {
	options.current_house_id = 0;
	Brush* brush = g_gui.GetCurrentBrush();
	if (brush) {
		if (brush->is<HouseBrush>()) {
			options.current_house_id = brush->as<HouseBrush>()->getHouseID();
		} else if (brush->is<HouseExitBrush>()) {
			options.current_house_id = brush->as<HouseExitBrush>()->getHouseID();
		}
	}

	// Calculate pulse for house highlighting
	// Period is 1 second (1000ms)
	// Range is [0.0, 1.0]
	// Using a sine wave for smooth transition
	// (sin(t) + 1) / 2
	double now = wxGetLocalTimeMillis().ToDouble();
	const double speed = 0.005;
	options.highlight_pulse = (float)((sin(now * speed) + 1.0) / 2.0);

	view.Setup(canvas, options);
}

void MapDrawer::SetupGL() {
	// GL viewport setup — separated from RenderView (pure data)
	glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	view.ComputeProjection();

	// Ensure renderers are initialized
	if (!renderers_initialized) {
		sprite_batch->initialize();
		primitive_renderer->initialize();
		post_process_pipeline->Initialize(kScreenVertFallback);
		renderers_initialized = true;
	}
}

void MapDrawer::Release() {
	// tooltip_drawer->clear(); // Moved to ClearTooltips(), called explicitly after UI draw
}

void MapDrawer::Draw() {
	g_gui.gfx.updateTime();

	light_buffer.Clear();
	creature_name_drawer->clear();
	options.transient_selection_bounds = std::nullopt;

	if (options.boundbox_selection) {
		options.transient_selection_bounds = MapBounds {
			.x1 = std::min(canvas->last_click_map_x, canvas->last_cursor_map_x),
			.y1 = std::min(canvas->last_click_map_y, canvas->last_cursor_map_y),
			.x2 = std::max(canvas->last_click_map_x, canvas->last_cursor_map_x),
			.y2 = std::max(canvas->last_click_map_y, canvas->last_cursor_map_y)
		};
	}

	if (!g_gui.gfx.ensureAtlasManager()) {
		return;
	}
	auto* atlas = g_gui.gfx.getAtlasManager();

	// Begin Batches
	sprite_batch->begin(view.projectionMatrix, *atlas);
	primitive_renderer->setProjectionMatrix(view.projectionMatrix);

	// Post-process pipeline handles FBO binding if needed
	bool use_fbo = post_process_pipeline->BeginCapture(view, options);

	DrawBackground(); // Clear screen (or FBO)

	const ViewBounds original_bounds { view.start_x, view.start_y, view.end_x, view.end_y };

	DrawMap();

	// Flush Map for Light Pass
	sprite_batch->end(*atlas);
	primitive_renderer->flush();

	if (options.isDrawLight()) {
		DrawLight();
	}

	// If using FBO, resolve to screen with post-process effect
	if (use_fbo) {
		post_process_pipeline->EndCaptureAndDraw(view, options);
	}

	// Resume Batch for Overlays
	sprite_batch->begin(view.projectionMatrix, *atlas);

	DrawContext ctx { *sprite_batch, view, options, editor };

	if (drag_shadow_drawer) {
		Position drag_start;
		if (canvas->selection_controller) {
			drag_start = canvas->selection_controller->GetDragStartPosition();
		}
		drag_shadow_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), drag_start);
	}

	live_cursor_drawer->draw(*sprite_batch, view, editor, options);

	bool is_dragging_draw = canvas->drawing_controller && canvas->drawing_controller->IsDraggingDraw();
	brush_overlay_drawer->draw(ctx, *primitive_renderer, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), brush_cursor_drawer.get(), is_dragging_draw, canvas->last_click_map_x, canvas->last_click_map_y);

	if (options.show_grid) {
		DrawGrid(original_bounds);
	}
	if (options.show_ingame_box) {
		DrawIngameBox(original_bounds);
	}

	// Draw creature names (Overlay) moved to DrawCreatureNames()

	// End Batches and Flush
	sprite_batch->end(*atlas);
	primitive_renderer->flush();

	// Tooltips are now drawn in MapCanvas::OnPaint (UI Pass)
}

void MapDrawer::DrawBackground() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MapDrawer::DrawMap() {
	bool live_client = editor.live_manager.IsClient();

	// Per-floor offsets computed as local variables — RenderView stays immutable
	int current_start_x = view.start_x;
	int current_start_y = view.start_y;
	int current_end_x = view.end_x;
	int current_end_y = view.end_y;

	for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
		if (map_z == view.end_z && view.start_z != view.end_z) {
			shade_drawer->draw(*sprite_batch, view, options);
		}

		if (map_z >= view.end_z) {
			DrawMapLayer(map_z, live_client);
		}

		preview_drawer->draw(*sprite_batch, canvas, view, map_z, options, editor, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), options.current_house_id);

		--current_start_x;
		--current_start_y;
		++current_end_x;
		++current_end_y;
	}
}

void MapDrawer::DrawIngameBox(const ViewBounds& bounds) {
	grid_drawer->DrawIngameBox(*sprite_batch, view, options, bounds);
}

void MapDrawer::DrawGrid(const ViewBounds& bounds) {
	grid_drawer->DrawGrid(*sprite_batch, view, options, bounds);
}

void MapDrawer::DrawTooltips(NVGcontext* vg) {
	tooltip_drawer->draw(vg, view);
}

void MapDrawer::DrawHookIndicators(NVGcontext* vg) {
	hook_indicator_drawer->draw(vg, view);
}

void MapDrawer::DrawDoorIndicators(NVGcontext* vg) {
	if (options.highlight_locked_doors) {
		door_indicator_drawer->draw(vg, view);
	}
}

void MapDrawer::DrawCreatureNames(NVGcontext* vg) {
	creature_name_drawer->draw(vg, view);
}

void MapDrawer::DrawMapLayer(int map_z, bool live_client) {
	map_layer_drawer->Draw(*sprite_batch, map_z, live_client, view, options, light_buffer);
}

void MapDrawer::DrawLight() {
	light_drawer->draw(view, options.experimental_fog, light_buffer, options.global_light_color, options.light_intensity, options.ambient_light_level);
}

void MapDrawer::TakeScreenshot(uint8_t* screenshot_buffer) {
	ScreenCapture::Capture(view.screensize_x, view.screensize_y, screenshot_buffer);
}

void MapDrawer::ClearFrameOverlays() {
	tooltip_drawer->clear();
	hook_indicator_drawer->clear();
	door_indicator_drawer->clear();
}
