#ifndef RME_RENDERING_DRAWING_OPTIONS_H_
#define RME_RENDERING_DRAWING_OPTIONS_H_

#include <cstdint>
#include <string>
#include <optional>
#include "map/position.h"
#include "app/definitions.h"

class Settings;

struct DrawingOptions {
	DrawingOptions();

	void SetIngame();
	void SetDefault();
	bool isDrawLight() const noexcept;

	// Factory: populates persistent user preferences from Settings + GUI values.
	// The global read stays at the UI call site; this struct has no global dependencies.
	static DrawingOptions FromSettings(const Settings& settings, float light_intensity, float ambient_light_level);

	// === Persistent user preferences (set from Settings) ===
	bool transparent_floors;
	bool transparent_items;
	bool show_ingame_box;
	bool show_lights;
	bool show_light_str;
	bool show_tech_items;
	bool show_waypoints;
	int show_grid;
	bool show_all_floors;
	bool show_creatures;
	bool show_spawns;
	bool show_houses;
	bool show_shade;
	bool show_special_tiles;
	bool show_items;

	bool highlight_items;
	bool highlight_locked_doors;
	bool show_blocking;
	bool show_tooltips;

	bool show_as_minimap;
	bool show_only_colors;
	bool show_only_modified;
	bool show_preview;
	bool show_hooks;
	bool hide_items_when_zoomed;
	bool show_towns;
	bool always_show_zones;
	bool extended_house_shader;

	bool experimental_fog;
	bool anti_aliasing;
	std::string screen_shader_name;

	float light_intensity;
	float ambient_light_level;

	// === Per-frame transient state (set each frame by MapDrawer/MapCanvas) ===
	bool ingame;
	bool dragging;
	bool boundbox_selection;
	uint32_t current_house_id;
	float highlight_pulse;
	DrawColor global_light_color { 128, 128, 128, 255 };
	std::optional<MapBounds> transient_selection_bounds;
};

#endif
