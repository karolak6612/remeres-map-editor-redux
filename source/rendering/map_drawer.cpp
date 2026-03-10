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
#include "brushes/brush_enums.h"
#include "rendering/drawers/map_layer_drawer.h"
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
#include "rendering/ui/tooltip_renderer.h"
#include "rendering/ui/nvg_image_cache.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"

#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/cursors/live_cursor_drawer.h"
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
#include "rendering/postprocess/post_process_pipeline.h"
#include "rendering/core/graphics_sprite_resolver.h"

MapDrawer::MapDrawer(Editor& editor) :
	editor(editor) {

	light_drawer = std::make_unique<LightDrawer>();

	sprite_drawer = std::make_unique<SpriteDrawer>();
	creature_drawer = std::make_unique<CreatureDrawer>();
	floor_drawer = std::make_unique<FloorDrawer>();
	item_drawer = std::make_unique<ItemDrawer>();
	marker_drawer = std::make_unique<MarkerDrawer>();

	creature_name_drawer = std::make_unique<CreatureNameDrawer>();

	tile_renderer = std::make_unique<TileRenderer>(TileRenderDeps {
		.item_drawer = item_drawer.get(),
		.sprite_drawer = sprite_drawer.get(),
		.creature_drawer = creature_drawer.get(),
		.marker_drawer = marker_drawer.get(),
		.editor = &editor
	});

	grid_drawer = std::make_unique<GridDrawer>();
	auto node_request_fn = [this](int x, int y, bool underground) {
		if (editor.live_manager.GetClient()) {
			editor.live_manager.GetClient()->queryNode(x, y, underground);
		}
	};
	map_layer_drawer = std::make_unique<MapLayerDrawer>(tile_renderer.get(), grid_drawer.get(), &editor, std::move(node_request_fn));
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
	post_process_ = std::make_unique<PostProcessPipeline>();

	sprite_resolver = std::make_unique<GraphicsSpriteResolver>(g_gui.gfx);
	item_drawer->SetSpriteResolver(sprite_resolver.get());
	creature_drawer->SetSpriteResolver(sprite_resolver.get());
	sprite_drawer->SetSpriteResolver(sprite_resolver.get());
}

MapDrawer::~MapDrawer() {

	Release();
}

void MapDrawer::SetupVars(const ViewSnapshot& snapshot) {
	snapshot_ = snapshot;

	frame_options.current_house_id = 0;
	Brush* brush = g_gui.GetCurrentBrush();
	if (brush) {
		if (brush->is<HouseBrush>()) {
			frame_options.current_house_id = brush->as<HouseBrush>()->getHouseID();
		} else if (brush->is<HouseExitBrush>()) {
			frame_options.current_house_id = brush->as<HouseExitBrush>()->getHouseID();
		}
	}

	// Calculate pulse for house highlighting
	double now = wxGetLocalTimeMillis().ToDouble();
	const double speed = 0.005;
	frame_options.highlight_pulse = (float)((sin(now * speed) + 1.0) / 2.0);

	// Construct ViewState from ViewSnapshot
	view.mouse_map_x = snapshot_.mouse_map_x;
	view.mouse_map_y = snapshot_.mouse_map_y;
	view.view_scroll_x = snapshot_.view_scroll_x;
	view.view_scroll_y = snapshot_.view_scroll_y;
	view.screensize_x = snapshot_.screensize_x;
	view.screensize_y = snapshot_.screensize_y;
	view.viewport_x = 0;
	view.viewport_y = 0;

	view.zoom = snapshot_.zoom;
	view.tile_size = std::max(1, static_cast<int>(TILE_SIZE / view.zoom));
	view.floor = snapshot_.floor;
	view.camera_pos.z = view.floor;

	if (render_settings.show_all_floors) {
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

	view.start_x = view.view_scroll_x / TILE_SIZE;
	view.start_y = view.view_scroll_y / TILE_SIZE;

	if (view.floor > GROUND_LAYER) {
		view.start_x -= 2;
		view.start_y -= 2;
	}

	view.end_x = view.start_x + view.screensize_x / view.tile_size + 2;
	view.end_y = view.start_y + view.screensize_y / view.tile_size + 2;

	view.logical_width = view.screensize_x * view.zoom;
	view.logical_height = view.screensize_y * view.zoom;
}

void MapDrawer::SetupGL() {
	GLViewport::Apply(view);
	ViewProjection::Compute(view);

	// Ensure renderers are initialized
	if (!renderers_initialized) {
		sprite_batch->initialize();
		primitive_renderer->initialize();
		renderers_initialized = true;
	}
}

void MapDrawer::Release() {
}

void MapDrawer::Draw() {
	g_gui.gfx.updateTime();

	light_buffer.Clear();
	frame_options.transient_selection_bounds = std::nullopt;

	if (frame_options.boundbox_selection) {
		frame_options.transient_selection_bounds = MapBounds {
			.x1 = std::min(snapshot_.last_click_map_x, snapshot_.last_cursor_map_x),
			.y1 = std::min(snapshot_.last_click_map_y, snapshot_.last_cursor_map_y),
			.x2 = std::max(snapshot_.last_click_map_x, snapshot_.last_cursor_map_x),
			.y2 = std::max(snapshot_.last_click_map_y, snapshot_.last_cursor_map_y)
		};
	}

	if (!g_gui.gfx.ensureAtlasManager()) {
		return;
	}
	current_atlas_ = g_gui.gfx.getAtlasManager();
	sprite_drawer->SetAtlas(current_atlas_);

	// Begin Batches
	sprite_batch->begin(view.projectionMatrix, *current_atlas_);
	primitive_renderer->setProjectionMatrix(view.projectionMatrix);

	// Post-processing: bind FBO if shader or AA is active
	bool use_fbo = post_process_->Begin(view, render_settings);

	DrawBackground(); // Clear screen (or FBO)

	DrawMap();

	// Flush Map for Light Pass
	sprite_batch->end(*current_atlas_);
	primitive_renderer->flush();

	if (render_settings.isDrawLight()) {
		DrawLight();
	}

	// If using FBO, resolve to screen
	if (use_fbo) {
		post_process_->End(view, render_settings);
	}

	// Resume Batch for Overlays
	sprite_batch->begin(view.projectionMatrix, *current_atlas_);

	const DrawContext ctx { *sprite_batch, *primitive_renderer, view, render_settings, frame_options, light_buffer, accumulators_, *current_atlas_ };

	if (drag_shadow_drawer) {
		drag_shadow_drawer->draw(ctx, editor, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(),
			snapshot_.drag_start);
	}

	live_cursor_drawer->draw(ctx, editor);

	{
		Brush* current_brush = g_gui.GetCurrentBrush();
		brush_overlay_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(),
			brush_cursor_drawer.get(), editor,
			g_gui.IsDrawingMode(), current_brush, g_gui.GetBrushShape(), g_gui.GetBrushSize(),
			snapshot_.is_dragging_draw,
			snapshot_.last_click_map_x, snapshot_.last_click_map_y);
	}

	const ViewBounds base_bounds { view.start_x, view.start_y, view.end_x, view.end_y };

	if (render_settings.show_grid) {
		DrawGrid(base_bounds);
	}
	if (render_settings.show_ingame_box) {
		DrawIngameBox(base_bounds);
	}

	// Draw creature names (Overlay) moved to DrawCreatureNames()

	// End Batches and Flush
	sprite_batch->end(*current_atlas_);
	primitive_renderer->flush();

	// Tooltips are now drawn in MapCanvas::OnPaint (UI Pass)
}

void MapDrawer::DrawBackground() {
	GLViewport::Clear();
}

void MapDrawer::DrawMap() {
	bool live_client = editor.live_manager.IsClient();

	const DrawContext ctx { *sprite_batch, *primitive_renderer, view, render_settings, frame_options, light_buffer, accumulators_, *current_atlas_ };

	int floor_offset = 0;

	for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
		FloorViewParams floor_params {
			view.start_x - floor_offset,
			view.start_y - floor_offset,
			view.end_x + floor_offset,
			view.end_y + floor_offset
		};

		if (map_z == view.end_z && view.start_z != view.end_z) {
			shade_drawer->draw(ctx);
		}

		if (map_z >= view.end_z) {
			DrawMapLayer(map_z, live_client, floor_params);
		}

		preview_drawer->draw(ctx, snapshot_, floor_params, map_z, editor, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), g_gui.GetCurrentBrush());

		++floor_offset;
	}
}

void MapDrawer::DrawIngameBox(const ViewBounds& bounds) {
	const DrawContext ctx { *sprite_batch, *primitive_renderer, view, render_settings, frame_options, light_buffer, accumulators_, *current_atlas_ };
	grid_drawer->DrawIngameBox(ctx, bounds);
}

void MapDrawer::DrawGrid(const ViewBounds& bounds) {
	const DrawContext ctx { *sprite_batch, *primitive_renderer, view, render_settings, frame_options, light_buffer, accumulators_, *current_atlas_ };
	grid_drawer->DrawGrid(ctx, bounds);
}

void MapDrawer::DrawTooltips(NVGcontext* vg) {
	tooltip_renderer.draw(vg, view, accumulators_.tooltips.getTooltips(), nvg_image_cache);
}

void MapDrawer::DrawHookIndicators(NVGcontext* vg) {
	hook_indicator_drawer->draw(vg, view, accumulators_.hooks);
}

void MapDrawer::DrawDoorIndicators(NVGcontext* vg) {
	if (render_settings.highlight_locked_doors) {
		door_indicator_drawer->draw(vg, view, accumulators_.doors);
	}
}

void MapDrawer::DrawCreatureNames(NVGcontext* vg) {
	creature_name_drawer->draw(vg, view, accumulators_.creature_names);
}

void MapDrawer::DrawMapLayer(int map_z, bool live_client, const FloorViewParams& floor_params) {
	const DrawContext ctx { *sprite_batch, *primitive_renderer, view, render_settings, frame_options, light_buffer, accumulators_, *current_atlas_ };
	map_layer_drawer->Draw(ctx, map_z, live_client, floor_params);
}

void MapDrawer::DrawLight() {
	light_drawer->draw(view, render_settings.experimental_fog, light_buffer, frame_options.global_light_color, render_settings.light_intensity, render_settings.ambient_light_level);
}

void MapDrawer::TakeScreenshot(uint8_t* screenshot_buffer) {
	ScreenCapture::Capture(view.screensize_x, view.screensize_y, screenshot_buffer);
}

void MapDrawer::ClearFrameOverlays() {
	accumulators_.clear();
}
