#include "ingame_preview/ingame_preview_renderer.h"
#include "game/creature.h"
#include "game/outfit.h"
#include "ingame_preview/floor_visibility_calculator.h"
#include "map/basemap.h"
#include "map/tile.h"
#include "rendering/core/canvas_state.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/text_renderer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/utilities/light_renderer.h"
#include "ui/gui.h"
#include <algorithm> // For std::max
#include <cmath>     // For std::floor, std::ceil
#include <cstdlib>   // For std::abs
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <nanovg.h>
#include <spdlog/spdlog.h>

namespace IngamePreview {

IngamePreviewRenderer::IngamePreviewRenderer(TileRenderer *tile_renderer)
    : tile_renderer(tile_renderer) {
  floor_calculator = std::make_unique<FloorVisibilityCalculator>();
  sprite_batch = std::make_unique<SpriteBatch>();
  primitive_renderer = std::make_unique<PrimitiveRenderer>();
  light_buffer = std::make_unique<LightBuffer>();
  light_renderer = std::make_unique<LightRenderer>();
  creature_drawer = std::make_unique<CreatureDrawer>();
  creature_name_drawer = std::make_unique<CreatureNameDrawer>();
  sprite_drawer = std::make_unique<SpriteDrawer>();

  if (!sprite_batch->initialize()) {
    spdlog::error("IngamePreviewRenderer failed to initialize sprite_batch");
    throw std::runtime_error("Failed to initialize SpriteBatch");
  }
  if (!primitive_renderer->initialize()) {
    spdlog::error(
        "IngamePreviewRenderer failed to initialize primitive_renderer");
    throw std::runtime_error("Failed to initialize PrimitiveRenderer");
  }
  last_time = std::chrono::steady_clock::now();

  // Initialize opacity
  for (int z = 0; z <= MAP_MAX_LAYER; ++z) {
    floor_opacity[z] = 0.0f;
  }
}

IngamePreviewRenderer::~IngamePreviewRenderer() = default;

void IngamePreviewRenderer::UpdateOpacity(double dt, int first_visible,
                                          int last_visible) {
  const float fade_speed = 4.0f; // Roughly 250ms for full fade
  for (int z = 0; z <= MAP_MAX_LAYER; ++z) {
    float target = (z >= first_visible && z <= last_visible) ? 1.0f : 0.0f;
    float current = floor_opacity[z];
    if (current < target) {
      current = std::min(target, current + static_cast<float>(dt * fade_speed));
    } else if (current > target) {
      current = std::max(target, current - static_cast<float>(dt * fade_speed));
    }
    floor_opacity[z] = current;
  }
}

void IngamePreviewRenderer::Render(
    NVGcontext *vg, const BaseMap &map, int viewport_x, int viewport_y,
    int viewport_width, int viewport_height, const Position &camera_pos,
    float zoom, bool lighting_enabled, uint8_t ambient_light,
    const Outfit &preview_outfit, Direction preview_direction,
    int animation_phase, int offset_x, int offset_y) {
  if (!tile_renderer) {
    spdlog::error(
        "IngamePreviewRenderer::Render called with null tile_renderer");
    return;
  }

  g_gui.gc.updateTime();
  auto now = std::chrono::steady_clock::now();
  double dt = std::chrono::duration<double>(now - last_time).count();
  last_time = now;

  int first_visible = floor_calculator->CalcFirstVisibleFloor(
      map, camera_pos.x, camera_pos.y, camera_pos.z);
  int last_visible = floor_calculator->CalcLastVisibleFloor(camera_pos.z);
  UpdateOpacity(dt, first_visible, last_visible);

  ViewState view;
  DrawingOptions options;
  SetupViewAndOptions(view, options, viewport_x, viewport_y, viewport_width,
                      viewport_height, camera_pos, zoom, lighting_enabled,
                      ambient_light, offset_x, offset_y);

  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  primitive_renderer->setProjectionMatrix(view.projectionMatrix);
  light_buffer->Clear();
  if (creature_name_drawer) {
    creature_name_drawer->clear();
  }

  if (!g_gui.atlas.ensureAtlasManager())
    return;
  auto *atlas = g_gui.atlas.getAtlasManager();

  CanvasState dummy_state{};
  DrawContext ctx{.state = {.view = view,
                            .options = options,
                            .canvas_state = dummy_state},
                  .backend = {.sprite_batch = *sprite_batch,
                              .primitive_renderer = *primitive_renderer,
                              .sprite_database = g_gui.sprites,
                              .atlas_manager = *atlas},
                  .output = {.light_buffer = *light_buffer,
                             .brush_cursor_drawer = nullptr}};

  int elevation_offset = GetTileElevationOffset(map.getTile(camera_pos));

  RenderFloors(ctx, map, last_visible, camera_pos, lighting_enabled, atlas);
  RenderPreviewCharacter(ctx, preview_outfit, preview_direction,
                         animation_phase, atlas, elevation_offset);

  if (lighting_enabled && light_renderer && options.settings.isDrawLight()) {
    light_renderer->draw(ctx);
  }

  RenderNames(vg, ctx, viewport_width, viewport_height, zoom, elevation_offset);
}

int IngamePreviewRenderer::GetTileElevationOffset(const Tile *tile) const {
  int elevation_offset = 0;
  if (tile) {
    for (const auto &item : tile->items) {
      elevation_offset += item->getHeight();
    }
    if (tile->ground) {
      elevation_offset += tile->ground->getHeight();
    }
    if (elevation_offset > 24) {
      elevation_offset = 24;
    }
  }
  return elevation_offset;
}

void IngamePreviewRenderer::SetupViewAndOptions(
    ViewState &view, DrawingOptions &options, int viewport_x, int viewport_y,
    int viewport_width, int viewport_height, const Position &camera_pos,
    float zoom, bool lighting_enabled, uint8_t ambient_light, int offset_x,
    int offset_y) {
  view.zoom = zoom;
  view.tile_size = TILE_SIZE;
  view.floor = camera_pos.z;
  view.screensize_x = viewport_width;
  view.screensize_y = viewport_height;
  view.camera_pos = camera_pos;
  view.viewport_x = viewport_x;
  view.viewport_y = viewport_y;

  view.logical_width = viewport_width * zoom;
  view.logical_height = viewport_height * zoom;

  int offset = (camera_pos.z <= GROUND_LAYER)
                   ? (GROUND_LAYER - camera_pos.z) * TILE_SIZE
                   : 0;
  view.view_scroll_x = (camera_pos.x * TILE_SIZE) + (TILE_SIZE / 2) - offset +
                       offset_x -
                       static_cast<int>(viewport_width * zoom / 2.0f);
  view.view_scroll_y = (camera_pos.y * TILE_SIZE) + (TILE_SIZE / 2) - offset +
                       offset_y -
                       static_cast<int>(viewport_height * zoom / 2.0f);

  view.projectionMatrix =
      glm::ortho(0.0f, static_cast<float>(viewport_width) * zoom,
                 static_cast<float>(viewport_height) * zoom, 0.0f, -1.0f, 1.0f);
  view.viewMatrix =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.375f, 0.375f, 0.0f));

  options.settings = RenderSettings::makeIngame();
  options.settings.show_lights = lighting_enabled;
  options.settings.ambient_light_level =
      static_cast<float>(ambient_light) / 255.0f;
  options.settings.light_intensity = light_intensity;
  options.settings.global_light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void IngamePreviewRenderer::RenderFloors(DrawContext &ctx, const BaseMap &map,
                                         int last_visible,
                                         const Position &camera_pos,
                                         bool lighting_enabled,
                                         AtlasManager *atlas) {
  for (int z = last_visible; z >= 0; z--) {
    float alpha = floor_opacity[z];
    if (alpha <= 0.001f) {
      continue;
    }

    sprite_batch->begin(ctx.state.view.projectionMatrix, *atlas);
    sprite_batch->setGlobalTint(1.0f, 1.0f, 1.0f, alpha, *atlas);

    int floor_offset = ctx.state.view.CalculateLayerOffset(z);
    constexpr int margin = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS;
    int start_x = static_cast<int>(
        std::floor((ctx.state.view.view_scroll_x + floor_offset - margin) /
                   static_cast<float>(TILE_SIZE)));
    int start_y = static_cast<int>(
        std::floor((ctx.state.view.view_scroll_y + floor_offset - margin) /
                   static_cast<float>(TILE_SIZE)));
    int end_x = static_cast<int>(
        std::ceil((ctx.state.view.view_scroll_x + floor_offset +
                   ctx.state.view.screensize_x * ctx.state.view.zoom + margin) /
                  static_cast<float>(TILE_SIZE)));
    int end_y = static_cast<int>(
        std::ceil((ctx.state.view.view_scroll_y + floor_offset +
                   ctx.state.view.screensize_y * ctx.state.view.zoom + margin) /
                  static_cast<float>(TILE_SIZE)));

    int base_draw_x = -ctx.state.view.view_scroll_x - floor_offset;
    int base_draw_y = -ctx.state.view.view_scroll_y - floor_offset;

    for (int x = start_x; x <= end_x; ++x) {
      for (int y = start_y; y <= end_y; ++y) {
        const Tile *tile = map.getTile(x, y, z);
        if (tile) {
          int draw_x = (x * TILE_SIZE) + base_draw_x;
          int draw_y = (y * TILE_SIZE) + base_draw_y;
          tile_renderer->DrawTile(ctx, tile->location, 0, draw_x, draw_y,
                                  lighting_enabled);

          if (creature_name_drawer && z == camera_pos.z) {
            if (tile->creature) {
              creature_name_drawer->addLabel(tile->location->getPosition(),
                                             tile->creature->getName(),
                                             tile->creature.get());
            }
          }
        }
      }
    }
    sprite_batch->end(*atlas);
  }
}

void IngamePreviewRenderer::RenderPreviewCharacter(
    DrawContext &ctx, const Outfit &preview_outfit, Direction preview_direction,
    int animation_phase, AtlasManager *atlas, int elevation_offset) {
  sprite_batch->begin(ctx.state.view.projectionMatrix, *atlas);

  int center_x =
      static_cast<int>((ctx.state.view.screensize_x * ctx.state.view.zoom) / 2.0f);
  int center_y =
      static_cast<int>((ctx.state.view.screensize_y * ctx.state.view.zoom) / 2.0f);

  int draw_x = center_x - 16;
  int draw_y = center_y - 16 - elevation_offset;

  creature_drawer->BlitCreature(
      ctx, sprite_drawer.get(), draw_x, draw_y, preview_outfit,
      preview_direction,
      CreatureDrawOptions{.animationPhase = animation_phase});

  sprite_batch->end(*atlas);
}

void IngamePreviewRenderer::RenderNames(NVGcontext *vg, const DrawContext &ctx,
                                        int viewport_width, int viewport_height,
                                        float zoom, int elevation_offset) {
  if (!creature_name_drawer || !vg) {
    return;
  }

  TextRenderer::BeginFrame(vg, viewport_width, viewport_height, 1.0f);
  creature_name_drawer->draw(vg, ctx);

  nvgSave(vg);
  float fontSize = 11.0f;
  nvgFontSize(vg, fontSize);
  nvgFontFace(vg, "sans");
  nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

  float screenCenterX = static_cast<float>(viewport_width) / 2.0f;
  float screenCenterY = static_cast<float>(viewport_height) / 2.0f;

  float labelY = screenCenterY -
                 (16.0f + static_cast<float>(elevation_offset)) / zoom - 2.0f;
  std::string name = preview_name;
  float textBounds[4];
  nvgTextBounds(vg, 0, 0, name.c_str(), nullptr, textBounds);
  float textWidth = textBounds[2] - textBounds[0];
  float textHeight = textBounds[3] - textBounds[1];
  float paddingX = 4.0f;
  float paddingY = 2.0f;

  nvgBeginPath(vg);
  nvgRoundedRect(vg, screenCenterX - textWidth / 2.0f - paddingX,
                 labelY - textHeight - paddingY * 2.0f,
                 textWidth + paddingX * 2.0f, textHeight + paddingY * 2.0f,
                 3.0f);
  nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
  nvgFill(vg);

  nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
  nvgText(vg, screenCenterX, labelY - paddingY, name.c_str(), nullptr);
  nvgRestore(vg);

  TextRenderer::EndFrame(vg);
}

} // namespace IngamePreview
