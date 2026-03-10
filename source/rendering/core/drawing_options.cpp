#include "app/main.h"
#include "rendering/core/drawing_options.h"
#include "app/settings.h"

DrawingOptions::DrawingOptions() {
	SetDefault();
}

void DrawingOptions::SetDefault() {
	transparent_floors = false;
	transparent_items = false;
	show_ingame_box = false;
	show_lights = false;
	show_light_str = true;
	show_tech_items = true;
	show_waypoints = true;
	ingame = false;
	dragging = false;
	boundbox_selection = false;

	show_grid = 0;
	show_all_floors = true;
	show_creatures = true;
	show_spawns = true;
	show_houses = true;
	show_shade = true;
	show_special_tiles = true;
	show_items = true;

	highlight_items = false;
	highlight_locked_doors = true;
	show_blocking = false;
	show_tooltips = false;
	show_as_minimap = false;
	show_only_colors = false;
	show_only_modified = false;
	show_preview = false;
	show_hooks = false;
	hide_items_when_zoomed = true;
	current_house_id = 0;
	light_intensity = 1.0f;
	ambient_light_level = 0.5f;
	global_light_color = DrawColor(128, 128, 128, 255);
	highlight_pulse = 0.0f;
	anti_aliasing = false;
	screen_shader_name = ShaderNames::NONE;
}

void DrawingOptions::SetIngame() {
	transparent_floors = false;
	transparent_items = false;
	show_ingame_box = false;
	show_lights = false;
	show_light_str = false;
	show_tech_items = false;
	show_waypoints = false;
	ingame = true;
	dragging = false;
	boundbox_selection = false;

	show_grid = 0;
	show_all_floors = true;
	show_creatures = true;
	show_spawns = false;
	show_houses = false;
	show_shade = false;
	show_special_tiles = false;
	show_items = true;

	highlight_items = false;
	highlight_locked_doors = false;
	show_blocking = false;
	show_tooltips = false;
	show_as_minimap = false;
	show_only_colors = false;
	show_only_modified = false;
	show_preview = false;
	show_hooks = false;
	hide_items_when_zoomed = false;
	current_house_id = 0;
}

DrawingOptions DrawingOptions::FromSettings(const Settings& settings, float gui_light_intensity, float gui_ambient_light_level) {
	DrawingOptions opts;
	opts.transparent_floors = settings.getBoolean(Config::TRANSPARENT_FLOORS);
	opts.transparent_items = settings.getBoolean(Config::TRANSPARENT_ITEMS);
	opts.show_ingame_box = settings.getBoolean(Config::SHOW_INGAME_BOX);
	opts.show_lights = settings.getBoolean(Config::SHOW_LIGHTS);
	opts.show_light_str = settings.getBoolean(Config::SHOW_LIGHT_STR);
	opts.show_tech_items = settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS);
	opts.show_waypoints = settings.getBoolean(Config::SHOW_WAYPOINTS);
	opts.show_grid = settings.getInteger(Config::SHOW_GRID);
	opts.ingame = !settings.getBoolean(Config::SHOW_EXTRA);
	opts.show_all_floors = settings.getBoolean(Config::SHOW_ALL_FLOORS);
	opts.show_creatures = settings.getBoolean(Config::SHOW_CREATURES);
	opts.show_spawns = settings.getBoolean(Config::SHOW_SPAWNS);
	opts.show_houses = settings.getBoolean(Config::SHOW_HOUSES);
	opts.show_shade = settings.getBoolean(Config::SHOW_SHADE);
	opts.show_special_tiles = settings.getBoolean(Config::SHOW_SPECIAL_TILES);
	opts.show_items = settings.getBoolean(Config::SHOW_ITEMS);
	opts.highlight_items = settings.getBoolean(Config::HIGHLIGHT_ITEMS);
	opts.highlight_locked_doors = settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS);
	opts.show_blocking = settings.getBoolean(Config::SHOW_BLOCKING);
	opts.show_tooltips = settings.getBoolean(Config::SHOW_TOOLTIPS);
	opts.show_as_minimap = settings.getBoolean(Config::SHOW_AS_MINIMAP);
	opts.show_only_colors = settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
	opts.show_only_modified = settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
	opts.show_preview = settings.getBoolean(Config::SHOW_PREVIEW);
	opts.show_hooks = settings.getBoolean(Config::SHOW_WALL_HOOKS);
	opts.hide_items_when_zoomed = settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
	opts.show_towns = settings.getBoolean(Config::SHOW_TOWNS);
	opts.always_show_zones = settings.getBoolean(Config::ALWAYS_SHOW_ZONES);
	opts.extended_house_shader = settings.getBoolean(Config::EXT_HOUSE_SHADER);
	opts.light_intensity = gui_light_intensity;
	opts.ambient_light_level = gui_ambient_light_level;

	opts.experimental_fog = settings.getBoolean(Config::EXPERIMENTAL_FOG);
	opts.anti_aliasing = settings.getBoolean(Config::ANTI_ALIASING);
	opts.screen_shader_name = settings.getString(Config::SCREEN_SHADER);
	return opts;
}

bool DrawingOptions::isDrawLight() const noexcept {
	return show_lights;
}
