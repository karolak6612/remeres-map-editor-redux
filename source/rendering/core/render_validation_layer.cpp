#include "app/main.h"

#include "rendering/core/render_validation_layer.h"

#include "rendering/core/draw_command_queue.h"
#include "rendering/map_drawer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <variant>

namespace {
	struct ViewSummary {
		float zoom = 0.0f;
		int tile_size = 0;
		int floor = 0;
		int start_x = 0;
		int start_y = 0;
		int start_z = 0;
		int end_x = 0;
		int end_y = 0;
		int end_z = 0;
		int superend_z = 0;
		int view_scroll_x = 0;
		int view_scroll_y = 0;
		int screensize_x = 0;
		int screensize_y = 0;
		int viewport_x = 0;
		int viewport_y = 0;
		int mouse_map_x = 0;
		int mouse_map_y = 0;
	};

	struct SettingsSummary {
		bool transparent_floors = false;
		bool transparent_items = false;
		bool show_ingame_box = false;
		bool show_lights = false;
		bool show_grid = false;
		bool show_all_floors = false;
		bool show_creatures = false;
		bool show_spawns = false;
		bool show_houses = false;
		bool show_shade = false;
		bool show_special_tiles = false;
		bool show_items = false;
		bool highlight_items = false;
		bool highlight_locked_doors = false;
		bool show_blocking = false;
		bool show_tooltips = false;
		bool show_as_minimap = false;
		bool show_only_colors = false;
		bool show_only_modified = false;
		bool show_preview = false;
		bool show_hooks = false;
		bool hide_items_when_zoomed = false;
		bool show_towns = false;
		bool always_show_zones = false;
		bool extended_house_shader = false;
		bool experimental_fog = false;
		bool anti_aliasing = false;
		float light_intensity = 0.0f;
		float ambient_light_level = 0.0f;
		bool ingame = false;
		std::string screen_shader_name;
	};

	struct OptionsSummary {
		uint32_t current_house_id = 0;
		float highlight_pulse = 0.0f;
		bool dragging = false;
		bool boundbox_selection = false;
		DrawColor global_light_color {0, 0, 0, 0};
		std::optional<MapBounds> transient_selection_bounds;
	};

	struct FloorSummary {
		int map_z = 0;
		size_t command_count = 0;

		[[nodiscard]] bool operator==(const FloorSummary& other) const
		{
			return map_z == other.map_z && command_count == other.command_count;
		}
	};

	struct PreparedFrameSummary {
		uint64_t generation = 0;
		uint32_t atlas_version = 0;
		ViewSummary view;
		SettingsSummary settings;
		OptionsSummary options;
		size_t floor_count = 0;
		size_t total_commands = 0;
		size_t light_count = 0;
		size_t hook_count = 0;
		size_t door_count = 0;
		size_t creature_name_count = 0;
		size_t tooltip_count = 0;
		size_t preload_request_count = 0;
		std::array<size_t, std::variant_size_v<DrawCommand>> command_type_counts {};
		std::vector<FloorSummary> floors;
	};

	[[nodiscard]] ViewSummary SummarizeView(const ViewState& view)
	{
		return ViewSummary {
			.zoom = view.zoom,
			.tile_size = view.tile_size,
			.floor = view.floor,
			.start_x = view.start_x,
			.start_y = view.start_y,
			.start_z = view.start_z,
			.end_x = view.end_x,
			.end_y = view.end_y,
			.end_z = view.end_z,
			.superend_z = view.superend_z,
			.view_scroll_x = view.view_scroll_x,
			.view_scroll_y = view.view_scroll_y,
			.screensize_x = view.screensize_x,
			.screensize_y = view.screensize_y,
			.viewport_x = view.viewport_x,
			.viewport_y = view.viewport_y,
			.mouse_map_x = view.mouse_map_x,
			.mouse_map_y = view.mouse_map_y,
		};
	}

	[[nodiscard]] SettingsSummary SummarizeSettings(const RenderSettings& settings)
	{
		return SettingsSummary {
			.transparent_floors = settings.transparent_floors,
			.transparent_items = settings.transparent_items,
			.show_ingame_box = settings.show_ingame_box,
			.show_lights = settings.show_lights,
			.show_grid = settings.show_grid != 0,
			.show_all_floors = settings.show_all_floors,
			.show_creatures = settings.show_creatures,
			.show_spawns = settings.show_spawns,
			.show_houses = settings.show_houses,
			.show_shade = settings.show_shade,
			.show_special_tiles = settings.show_special_tiles,
			.show_items = settings.show_items,
			.highlight_items = settings.highlight_items,
			.highlight_locked_doors = settings.highlight_locked_doors,
			.show_blocking = settings.show_blocking,
			.show_tooltips = settings.show_tooltips,
			.show_as_minimap = settings.show_as_minimap,
			.show_only_colors = settings.show_only_colors,
			.show_only_modified = settings.show_only_modified,
			.show_preview = settings.show_preview,
			.show_hooks = settings.show_hooks,
			.hide_items_when_zoomed = settings.hide_items_when_zoomed,
			.show_towns = settings.show_towns,
			.always_show_zones = settings.always_show_zones,
			.extended_house_shader = settings.extended_house_shader,
			.experimental_fog = settings.experimental_fog,
			.anti_aliasing = settings.anti_aliasing,
			.light_intensity = settings.light_intensity,
			.ambient_light_level = settings.ambient_light_level,
			.ingame = settings.ingame,
			.screen_shader_name = settings.screen_shader_name,
		};
	}

	[[nodiscard]] OptionsSummary SummarizeOptions(const FrameOptions& options)
	{
		return OptionsSummary {
			.current_house_id = options.current_house_id,
			.highlight_pulse = options.highlight_pulse,
			.dragging = options.dragging,
			.boundbox_selection = options.boundbox_selection,
			.global_light_color = options.global_light_color,
			.transient_selection_bounds = options.transient_selection_bounds,
		};
	}

	[[nodiscard]] PreparedFrameSummary SummarizePreparedFrame(const PreparedFrameBuffer& prepared)
	{
		PreparedFrameSummary summary;
		summary.generation = prepared.generation;
		summary.atlas_version = prepared.atlas_version;
		summary.view = SummarizeView(prepared.frame.view);
		summary.settings = SummarizeSettings(prepared.frame.settings);
		summary.options = SummarizeOptions(prepared.frame.options);
		summary.floor_count = prepared.floor_ranges.size();
		summary.total_commands = prepared.commands.size();
		summary.light_count = prepared.lights.size();
		summary.hook_count = prepared.accumulators.hooks.size();
		summary.door_count = prepared.accumulators.doors.size();
		summary.creature_name_count = prepared.accumulators.creature_names.size();
		summary.tooltip_count = prepared.accumulators.tooltips.count();
		summary.preload_request_count = prepared.preload_requests.size();
		summary.floors.reserve(prepared.floor_ranges.size());
		for (const auto& floor : prepared.floor_ranges) {
			summary.floors.push_back(FloorSummary {.map_z = floor.map_z, .command_count = floor.command_count});
		}
		for (const auto& command : prepared.commands.commands()) {
			summary.command_type_counts[command.index()] += 1;
		}
		return summary;
	}

	[[nodiscard]] bool NearlyEqual(float lhs, float rhs)
	{
		return std::fabs(lhs - rhs) <= 0.0001f;
	}

	[[nodiscard]] bool EqualBounds(const std::optional<MapBounds>& lhs, const std::optional<MapBounds>& rhs)
	{
		if (lhs.has_value() != rhs.has_value()) {
			return false;
		}
		if (!lhs.has_value()) {
			return true;
		}
		return lhs->x1 == rhs->x1 && lhs->y1 == rhs->y1 && lhs->x2 == rhs->x2 && lhs->y2 == rhs->y2;
	}

	[[nodiscard]] std::optional<std::string> CompareSummaries(const PreparedFrameSummary& lhs, const PreparedFrameSummary& rhs)
	{
		if (lhs.atlas_version != rhs.atlas_version) {
			return "atlas version differs";
		}
		if (lhs.floor_count != rhs.floor_count) {
			return "floor count differs";
		}
		if (lhs.total_commands != rhs.total_commands) {
			return "total command count differs";
		}
		if (lhs.light_count != rhs.light_count) {
			return "light count differs";
		}
		if (lhs.hook_count != rhs.hook_count) {
			return "hook count differs";
		}
		if (lhs.door_count != rhs.door_count) {
			return "door count differs";
		}
		if (lhs.creature_name_count != rhs.creature_name_count) {
			return "creature label count differs";
		}
		if (lhs.tooltip_count != rhs.tooltip_count) {
			return "tooltip count differs";
		}
		if (lhs.preload_request_count != rhs.preload_request_count) {
			return "preload request count differs";
		}
		if (lhs.command_type_counts != rhs.command_type_counts) {
			return "draw command mix differs";
		}
		if (lhs.floors != rhs.floors) {
			return "per-floor command ranges differ";
		}

		const auto& lview = lhs.view;
		const auto& rview = rhs.view;
		if (!NearlyEqual(lview.zoom, rview.zoom) || lview.tile_size != rview.tile_size || lview.floor != rview.floor
			|| lview.start_x != rview.start_x || lview.start_y != rview.start_y || lview.start_z != rview.start_z || lview.end_x != rview.end_x
			|| lview.end_y != rview.end_y || lview.end_z != rview.end_z || lview.superend_z != rview.superend_z
			|| lview.view_scroll_x != rview.view_scroll_x || lview.view_scroll_y != rview.view_scroll_y
			|| lview.screensize_x != rview.screensize_x || lview.screensize_y != rview.screensize_y || lview.viewport_x != rview.viewport_x
			|| lview.viewport_y != rview.viewport_y || lview.mouse_map_x != rview.mouse_map_x || lview.mouse_map_y != rview.mouse_map_y) {
			return "view state differs";
		}

		const auto& lsettings = lhs.settings;
		const auto& rsettings = rhs.settings;
		if (lsettings.transparent_floors != rsettings.transparent_floors || lsettings.transparent_items != rsettings.transparent_items
			|| lsettings.show_ingame_box != rsettings.show_ingame_box || lsettings.show_lights != rsettings.show_lights
			|| lsettings.show_grid != rsettings.show_grid || lsettings.show_all_floors != rsettings.show_all_floors
			|| lsettings.show_creatures != rsettings.show_creatures || lsettings.show_spawns != rsettings.show_spawns
			|| lsettings.show_houses != rsettings.show_houses || lsettings.show_shade != rsettings.show_shade
			|| lsettings.show_special_tiles != rsettings.show_special_tiles || lsettings.show_items != rsettings.show_items
			|| lsettings.highlight_items != rsettings.highlight_items || lsettings.highlight_locked_doors != rsettings.highlight_locked_doors
			|| lsettings.show_blocking != rsettings.show_blocking || lsettings.show_tooltips != rsettings.show_tooltips
			|| lsettings.show_as_minimap != rsettings.show_as_minimap || lsettings.show_only_colors != rsettings.show_only_colors
			|| lsettings.show_only_modified != rsettings.show_only_modified || lsettings.show_preview != rsettings.show_preview
			|| lsettings.show_hooks != rsettings.show_hooks || lsettings.hide_items_when_zoomed != rsettings.hide_items_when_zoomed
			|| lsettings.show_towns != rsettings.show_towns || lsettings.always_show_zones != rsettings.always_show_zones
			|| lsettings.extended_house_shader != rsettings.extended_house_shader || lsettings.experimental_fog != rsettings.experimental_fog
			|| lsettings.anti_aliasing != rsettings.anti_aliasing || !NearlyEqual(lsettings.light_intensity, rsettings.light_intensity)
			|| !NearlyEqual(lsettings.ambient_light_level, rsettings.ambient_light_level) || lsettings.ingame != rsettings.ingame
			|| lsettings.screen_shader_name != rsettings.screen_shader_name) {
			return "render settings differ";
		}

		const auto& loptions = lhs.options;
		const auto& roptions = rhs.options;
		if (loptions.current_house_id != roptions.current_house_id || !NearlyEqual(loptions.highlight_pulse, roptions.highlight_pulse)
			|| loptions.dragging != roptions.dragging || loptions.boundbox_selection != roptions.boundbox_selection
			|| loptions.global_light_color.r != roptions.global_light_color.r || loptions.global_light_color.g != roptions.global_light_color.g
			|| loptions.global_light_color.b != roptions.global_light_color.b || loptions.global_light_color.a != roptions.global_light_color.a
			|| !EqualBounds(loptions.transient_selection_bounds, roptions.transient_selection_bounds)) {
			return "frame options differ";
		}

		return std::nullopt;
	}
}

RenderValidationLayer::RenderValidationLayer(size_t sample_interval) : sample_interval_(std::max<size_t>(1, sample_interval))
{
}

bool RenderValidationLayer::shouldSample(uint64_t generation) const
{
	return generation % sample_interval_ == 0;
}

void RenderValidationLayer::trackSnapshot(const RenderPrepSnapshot& snapshot)
{
	sampled_snapshots_.push_back(snapshot);
	if (sampled_snapshots_.size() > 8) {
		sampled_snapshots_.pop_front();
	}
}

std::optional<RenderValidationResult> RenderValidationLayer::validatePreparedFrame(MapDrawer& drawer, const PreparedFrameBuffer& prepared)
{
	const auto it = std::find_if(sampled_snapshots_.begin(), sampled_snapshots_.end(), [&](const RenderPrepSnapshot& snapshot) {
		return snapshot.generation == prepared.generation;
	});
	if (it == sampled_snapshots_.end()) {
		return std::nullopt;
	}

	RenderPrepSnapshot snapshot = *it;
	sampled_snapshots_.erase(it);

	PreparedFrameBuffer reference = drawer.PrepareFrame(std::move(snapshot));
	const auto threaded_summary = SummarizePreparedFrame(prepared);
	const auto reference_summary = SummarizePreparedFrame(reference);
	if (const auto mismatch = CompareSummaries(threaded_summary, reference_summary)) {
		std::ostringstream stream;
		stream << "validation mismatch: " << *mismatch;
		return RenderValidationResult {
			.ok = false,
			.generation = prepared.generation,
			.message = stream.str(),
		};
	}

	return RenderValidationResult {
		.ok = true,
		.generation = prepared.generation,
		.message = "validation passed",
	};
}

void RenderValidationLayer::discardOlderThan(uint64_t generation)
{
	std::erase_if(sampled_snapshots_, [&](const RenderPrepSnapshot& snapshot) { return snapshot.generation < generation; });
}

void RenderValidationLayer::clear()
{
	sampled_snapshots_.clear();
}
