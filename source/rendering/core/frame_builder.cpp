#include "rendering/core/frame_builder.h"

#include "app/definitions.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "editor/editor.h"

#include <algorithm>

DrawFrame FrameBuilder::Build(
	const ViewSnapshot& snapshot,
	const BrushSnapshot& brush,
	const RenderSettings& settings,
	const FrameOptions& base_options,
	const Editor& editor,
	AtlasManager* atlas
) {
	DrawFrame frame;
	frame.settings = settings;
	frame.snapshot = snapshot;
	frame.brush = brush;
	frame.options = ComputeFrameOptions(base_options, brush, editor);
	frame.view = ComputeViewState(snapshot, settings);
	frame.atlas = atlas;
	frame.lights.reserve(512);
	return frame;
}

ViewState FrameBuilder::ComputeViewState(const ViewSnapshot& snapshot, const RenderSettings& settings) {
	ViewState view;
	view.mouse_map_x = snapshot.mouse_map_x;
	view.mouse_map_y = snapshot.mouse_map_y;
	view.view_scroll_x = snapshot.view_scroll_x;
	view.view_scroll_y = snapshot.view_scroll_y;
	view.screensize_x = snapshot.screensize_x;
	view.screensize_y = snapshot.screensize_y;
	view.viewport_x = 0;
	view.viewport_y = 0;

	view.zoom = snapshot.zoom;
	view.tile_size = std::max(1, static_cast<int>(TILE_SIZE / view.zoom));
	view.floor = snapshot.floor;
	view.camera_pos.z = view.floor;

	if (settings.show_all_floors) {
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
	return view;
}

FrameOptions FrameBuilder::ComputeFrameOptions(const FrameOptions& base_options, const BrushSnapshot& brush, const Editor& editor) {
	static_cast<void>(editor);

	FrameOptions options = base_options;
	options.current_house_id = 0;
	if (brush.current_brush) {
		if (brush.current_brush->is<HouseBrush>()) {
			options.current_house_id = brush.current_brush->as<HouseBrush>()->getHouseID();
		} else if (brush.current_brush->is<HouseExitBrush>()) {
			options.current_house_id = brush.current_brush->as<HouseExitBrush>()->getHouseID();
		}
	}

	options.highlight_pulse = FrameOptions::ComputeHighlightPulse(wxGetLocalTimeMillis().ToDouble());
	return options;
}
