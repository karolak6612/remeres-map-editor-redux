#include "rendering/core/drawing_options.h"
#include "app/settings.h"
#include "rendering/postprocess/post_process_manager.h"

RenderSettings RenderSettings::makeDefault() {
  RenderSettings s;
  s.transparent_floors = false;
  s.transparent_items = false;
  s.show_ingame_box = false;
  s.show_lights = false;
  s.show_light_str = true;
  s.show_tech_items = true;
  s.show_waypoints = true;
  s.ingame = false;

  s.show_grid = 0;
  s.show_all_floors = true;
  s.show_creatures = true;
  s.show_spawns = true;
  s.show_houses = true;
  s.show_shade = true;
  s.show_special_tiles = true;
  s.show_items = true;

  s.highlight_items = false;
  s.highlight_locked_doors = true;
  s.show_blocking = false;
  s.show_tooltips = false;
  s.show_as_minimap = false;
  s.show_only_colors = false;
  s.show_only_modified = false;
  s.show_preview = false;
  s.show_hooks = false;
  s.hide_items_when_zoomed = true;
  s.show_towns = false;
  s.always_show_zones = false;
  s.extended_house_shader = false;

  s.light_intensity = 1.0f;
  s.ambient_light_level = 0.5f;
  s.global_light_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  s.anti_aliasing = false;
  s.screen_shader_name = ShaderNames::NONE;
  s.experimental_fog = false;

  return s;
}

RenderSettings RenderSettings::makeIngame() {
  RenderSettings s;
  s.transparent_floors = false;
  s.transparent_items = false;
  s.show_ingame_box = false;
  s.show_lights = false;
  s.show_light_str = false;
  s.show_tech_items = false;
  s.show_waypoints = false;
  s.ingame = true;

  s.show_grid = 0;
  s.show_all_floors = true;
  s.show_creatures = true;
  s.show_spawns = false;
  s.show_houses = false;
  s.show_shade = false;
  s.show_special_tiles = false;
  s.show_items = true;

  s.highlight_items = false;
  s.highlight_locked_doors = false;
  s.show_blocking = false;
  s.show_tooltips = false;
  s.show_as_minimap = false;
  s.show_only_colors = false;
  s.show_only_modified = false;
  s.show_preview = false;
  s.show_hooks = false;
  s.hide_items_when_zoomed = false;
  s.show_towns = false;
  s.always_show_zones = false;
  s.extended_house_shader = false;

  s.light_intensity = 1.0f;
  s.ambient_light_level = 0.5f;
  s.ambient_light_level = 0.5f;
  s.global_light_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  s.anti_aliasing = false;
  s.screen_shader_name = ShaderNames::NONE;
  s.experimental_fog = false;

  return s;
}

RenderSettings buildRenderSettings(const Settings &s, float light_intensity,
                                   float ambient_light_level) {
  RenderSettings rs = RenderSettings::makeDefault();
  rs.transparent_floors = s.getBoolean(Config::TRANSPARENT_FLOORS);
  rs.transparent_items = s.getBoolean(Config::TRANSPARENT_ITEMS);
  rs.show_ingame_box = s.getBoolean(Config::SHOW_INGAME_BOX);
  rs.show_lights = s.getBoolean(Config::SHOW_LIGHTS);
  rs.show_light_str = s.getBoolean(Config::SHOW_LIGHT_STR);
  rs.show_tech_items = s.getBoolean(Config::SHOW_TECHNICAL_ITEMS);
  rs.show_waypoints = s.getBoolean(Config::SHOW_WAYPOINTS);
  rs.show_grid = s.getInteger(Config::SHOW_GRID);
  rs.ingame = !s.getBoolean(Config::SHOW_EXTRA);
  rs.show_all_floors = s.getBoolean(Config::SHOW_ALL_FLOORS);
  rs.show_creatures = s.getBoolean(Config::SHOW_CREATURES);
  rs.show_spawns = s.getBoolean(Config::SHOW_SPAWNS);
  rs.show_houses = s.getBoolean(Config::SHOW_HOUSES);
  rs.show_shade = s.getBoolean(Config::SHOW_SHADE);
  rs.show_special_tiles = s.getBoolean(Config::SHOW_SPECIAL_TILES);
  rs.show_items = s.getBoolean(Config::SHOW_ITEMS);
  rs.highlight_items = s.getBoolean(Config::HIGHLIGHT_ITEMS);
  rs.highlight_locked_doors = s.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS);
  rs.show_blocking = s.getBoolean(Config::SHOW_BLOCKING);
  rs.show_tooltips = s.getBoolean(Config::SHOW_TOOLTIPS);
  rs.show_as_minimap = s.getBoolean(Config::SHOW_AS_MINIMAP);
  rs.show_only_colors = s.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
  rs.show_only_modified = s.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
  rs.show_preview = s.getBoolean(Config::SHOW_PREVIEW);
  rs.show_hooks = s.getBoolean(Config::SHOW_WALL_HOOKS);
  rs.hide_items_when_zoomed = s.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
  rs.show_towns = s.getBoolean(Config::SHOW_TOWNS);
  rs.always_show_zones = s.getBoolean(Config::ALWAYS_SHOW_ZONES);
  rs.extended_house_shader = s.getBoolean(Config::EXT_HOUSE_SHADER);
  rs.experimental_fog = s.getBoolean(Config::EXPERIMENTAL_FOG);
  rs.anti_aliasing = s.getBoolean(Config::ANTI_ALIASING);
  rs.screen_shader_name = s.getString(Config::SCREEN_SHADER);

  rs.light_intensity = light_intensity;
  rs.ambient_light_level = ambient_light_level;
  // global_light_color intentionally left at default — it is computed
  // per-frame from the light system via colorFromEightBitNorm() in LightDrawer.

  return rs;
}
