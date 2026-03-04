#ifndef RME_RENDERING_DRAWING_OPTIONS_H_
#define RME_RENDERING_DRAWING_OPTIONS_H_

#include "map/position.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <string>


class Settings;

// Persistent user preferences — changes only when the user modifies a setting.
struct RenderSettings {
  bool transparent_floors = false;
  bool transparent_items = false;
  bool show_ingame_box = false;
  bool show_lights = false;
  bool show_light_str = true;
  bool show_tech_items = true;
  bool show_waypoints = true;
  bool ingame = false;

  int show_grid = 0;
  bool show_all_floors = true;
  bool show_creatures = true;
  bool show_spawns = true;
  bool show_houses = true;
  bool show_shade = true;
  bool show_special_tiles = true;
  bool show_items = true;

  bool highlight_items = false;
  bool highlight_locked_doors = true;
  bool show_blocking = false;
  bool show_tooltips = false;

  bool show_as_minimap = false;
  bool show_only_colors = false;
  bool show_only_modified = false;
  bool show_preview = false;
  bool show_hooks = false;
  bool hide_items_when_zoomed = true;
  bool show_towns = false;
  bool always_show_zones = false;
  bool extended_house_shader = false;

  bool experimental_fog = false;
  bool anti_aliasing = false;
  std::string screen_shader_name;

  glm::vec4 global_light_color{0.5f, 0.5f, 0.5f, 1.0f};
  float light_intensity = 1.0f;
  float ambient_light_level = 0.5f;

  [[nodiscard]] bool isDrawLight() const noexcept { return show_lights; }

  // Named factory: default editor settings
  [[nodiscard]] static RenderSettings makeDefault();

  // Named factory: ingame preview settings (no editor overlays)
  [[nodiscard]] static RenderSettings makeIngame();
};

// Transient per-frame state — recomputed fresh each frame.
struct FrameState {
  bool dragging = false;
  bool boundbox_selection = false;
  std::optional<MapBounds> transient_selection_bounds;
  uint32_t current_house_id = 0;
  float highlight_pulse = 0.0f;
};

// Aggregate: full drawing context passed through the render pipeline.
struct DrawingOptions {
  RenderSettings settings;
  FrameState frame;
};

// Factory: build RenderSettings from explicit parameters — no globals.
[[nodiscard]] RenderSettings buildRenderSettings(const Settings &s,
                                                 float light_intensity,
                                                 float ambient_light_level);

#endif
