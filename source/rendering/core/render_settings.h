#ifndef RME_RENDERING_CORE_RENDER_SETTINGS_H_
#define RME_RENDERING_CORE_RENDER_SETTINGS_H_

#include <cstdint>
#include <string>
#include "app/definitions.h"

class Settings;

// Persistent rendering settings populated from g_settings once per frame.
// Lifetime: frame-scoped, but values only change when user modifies preferences.
struct RenderSettings {
	RenderSettings();

	void SetIngame();
	void SetDefault();
	[[nodiscard]] bool isDrawLight() const noexcept;

	// Factory: populates from Settings + GUI values.
	static RenderSettings FromSettings(const Settings& settings, float light_intensity, float ambient_light_level);

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

	// Cursor colors (used by BrushOverlayDrawer)
	uint8_t cursor_red, cursor_green, cursor_blue, cursor_alpha;
	uint8_t cursor_alt_red, cursor_alt_green, cursor_alt_blue, cursor_alt_alpha;
	bool use_automagic;

	bool ingame;
};

#endif
