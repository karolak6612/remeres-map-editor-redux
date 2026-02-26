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
#include "rendering/core/shader_program.h"
#include "rendering/postprocess/post_process_manager.h"

// Shader Sources
const char* screen_vert = R"(
#version 450 core
layout(location = 0) in vec2 aPos; // -1..1
layout(location = 1) in vec2 aTexCoord; // 0..1

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
	creature_drawer = std::make_unique<CreatureDrawer>(sprite_drawer.get());
	floor_drawer = std::make_unique<FloorDrawer>();
	item_drawer = std::make_unique<ItemDrawer>(sprite_drawer.get(), creature_drawer.get());
	marker_drawer = std::make_unique<MarkerDrawer>();

	creature_name_drawer = std::make_unique<CreatureNameDrawer>();

	tile_renderer = std::make_unique<TileRenderer>(item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), creature_name_drawer.get(), floor_drawer.get(), marker_drawer.get());

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
	// Reset texture cache at the start of each frame
	sprite_drawer->ResetCache();

	view.SetupGL();

	// Ensure renderers are initialized
	if (!renderers_initialized) {

		sprite_batch->initialize();
		primitive_renderer->initialize();
		renderers_initialized = true;
	}

	post_process_pipeline->Initialize();
}

void MapDrawer::Release() {
	ClearCache();
	// tooltip_drawer->clear(); // Moved to ClearTooltips(), called explicitly after UI draw
}

void MapDrawer::ClearCache() {
	if (map_layer_drawer) {
		map_layer_drawer->ClearCache();
	}
}

void MapDrawer::Draw() {
	g_gui.gfx.updateTime();

	light_buffer.Clear();
	creature_name_drawer->clear();

	// Begin Batches
	sprite_batch->begin(view.projectionMatrix);
	primitive_renderer->setProjectionMatrix(view.projectionMatrix);

	// Check Framebuffer Logic
	bool use_fbo = post_process_pipeline->IsActive(options);

	if (use_fbo) {
		post_process_pipeline->UpdateFBO(view, options);
	}

	DrawBackground(); // Clear screen (or FBO)
	DrawMap();

	// Flush Map for Light Pass
	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_batch->end(*g_gui.gfx.getAtlasManager());
	}
	primitive_renderer->flush();

	if (options.isDrawLight()) {
		DrawLight();
	}

	// If using FBO, we must now Resolve to Screen
	if (use_fbo) {
		post_process_pipeline->Draw(view, options);
		// Reset to default FBO for overlays
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
	}

	// Resume Batch for Overlays
	sprite_batch->begin(view.projectionMatrix);

	if (drag_shadow_drawer) {
		drag_shadow_drawer->draw(*sprite_batch, this, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), view, options);
	}

	if (options.boundbox_selection) {
		selection_drawer->draw(*sprite_batch, view, canvas, options);
	}
	live_cursor_drawer->draw(*sprite_batch, view, editor, options);

	brush_overlay_drawer->draw(*sprite_batch, *primitive_renderer, this, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), view, options, editor);

	if (options.show_grid) {
		DrawGrid();
	}
	if (options.show_ingame_box) {
		DrawIngameBox();
	}

	// Draw creature names (Overlay) moved to DrawCreatureNames()

	// End Batches and Flush
	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_batch->end(*g_gui.gfx.getAtlasManager());
	}
	primitive_renderer->flush();

	// Tooltips are now drawn in MapCanvas::OnPaint (UI Pass)
}

void MapDrawer::DrawBackground() {
	view.Clear();
}

void MapDrawer::DrawMap() {
	bool live_client = editor.live_manager.IsClient();

	bool only_colors = options.show_as_minimap || options.show_only_colors;

	// Phase 1: Extraction
	// Evaluate all dirty chunks and bake them into display lists without making any GL calls.
	// We iterate backwards to match drawing order for consistency, though extraction order doesn't strictly matter.
	int current_start_x = view.start_x;
	int current_start_y = view.start_y;
	int current_end_x = view.end_x;
	int current_end_y = view.end_y;

	for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
		if (map_z >= view.end_z) {

			// We need to pass a copy of view to Extract since the view bounds strictly shrink/expand per layer
			RenderView extract_view = view;
			extract_view.start_x = current_start_x;
			extract_view.start_y = current_start_y;
			extract_view.end_x = current_end_x;
			extract_view.end_y = current_end_y;

			map_layer_drawer->Extract(map_z, live_client, extract_view, options);
		}

		--current_start_x;
		--current_start_y;
		++current_end_x;
		++current_end_y;
	}

	// Phase 2: Submission
	// Now that all RenderLists are prepared, rapidly dispatch them into the SpriteBatch.
	for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
		if (map_z == view.end_z && view.start_z != view.end_z) {
			shade_drawer->draw(*sprite_batch, view, options);
		}

		if (map_z >= view.end_z) {
			DrawMapLayer(map_z, live_client);
		}

		preview_drawer->draw(*sprite_batch, canvas, view, map_z, options, editor, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), options.current_house_id);

		--view.start_x;
		--view.start_y;
		++view.end_x;
		++view.end_y;
	}
}

void MapDrawer::DrawIngameBox() {
	grid_drawer->DrawIngameBox(*sprite_batch, view, options);
}

void MapDrawer::DrawGrid() {
	grid_drawer->DrawGrid(*sprite_batch, view, options);
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
