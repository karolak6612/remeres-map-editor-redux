#include "rendering/utilities/light_renderer.h"
#include "app/main.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/gl_scoped_state.h"
#include "rendering/core/view_state.h"
#include "rendering/core/light_fbo.h"
#include "rendering/core/light_shader.h"
#include "rendering/core/light_buffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

LightRenderer::LightRenderer() {}

LightRenderer::~LightRenderer() = default;

void LightRenderer::draw(const DrawContext &ctx) {
  if (!shader) {
    shader = std::make_unique<LightShader>();
    if (!shader->getShader()) {
      return;
    }
  }

  if (!fbo) {
    fbo = std::make_unique<LightFBO>();
  }

  const auto &view = ctx.view;
  const auto &light_buffer = ctx.light_buffer;
  const auto &global_color = ctx.options.settings.global_light_color;
  float light_intensity = ctx.options.settings.light_intensity;
  float ambient_light_level = ctx.options.settings.ambient_light_level;

  int buffer_w = view.screensize_x;
  int buffer_h = view.screensize_y;

  if (buffer_w <= 0 || buffer_h <= 0) {
    return;
  }

  // 1. Resize FBO if needed
  fbo->Resize(buffer_w, buffer_h);

  // 2. Prepare Lights
  gpu_lights_.clear();
  gpu_lights_.reserve(light_buffer.lights.size());

  for (const auto &light : light_buffer.lights) {
    int lx_px = light.map_x * TILE_SIZE + TILE_SIZE / 2;
    int ly_px = light.map_y * TILE_SIZE + TILE_SIZE / 2;

    float map_pos_x = static_cast<float>(lx_px - view.view_scroll_x);
    float map_pos_y = static_cast<float>(ly_px - view.view_scroll_y);

    // Convert to Screen Pixels
    float screen_x = map_pos_x / view.zoom;
    float screen_y = map_pos_y / view.zoom;
    float screen_radius = (light.intensity * TILE_SIZE) / view.zoom;

    // Check overlap with Screen Rect
    if (screen_x + screen_radius < 0 || screen_x - screen_radius > buffer_w ||
        screen_y + screen_radius < 0 || screen_y - screen_radius > buffer_h) {
      continue;
    }

    glm::vec4 c = colorFromEightBitNorm(light.color);

    gpu_lights_.push_back(
        {.position = {screen_x, screen_y},
         .intensity = static_cast<float>(light.intensity),
         .padding = 0.0f,
         // Pre-multiply intensity here if needed, or in shader
         .color = {c.r * light_intensity, c.g * light_intensity,
                   c.b * light_intensity, 1.0f}});
  }

  if (!gpu_lights_.empty()) {
    shader->Upload(gpu_lights_);
  }

  // 3. Render to FBO
  {
    ScopedGLFramebuffer fbo_scope(GL_FRAMEBUFFER, fbo->getFBO()->GetID());
    ScopedGLViewport viewport_scope(0, 0, buffer_w, buffer_h);

    // Clear to Ambient Color
    float ambient_r = global_color.r * ambient_light_level;
    float ambient_g = global_color.g * ambient_light_level;
    float ambient_b = global_color.b * ambient_light_level;

    // If global_color is (0,0,0) (not set), use a default dark ambient
    if (global_color.r == 0.0f && global_color.g == 0.0f &&
        global_color.b == 0.0f) {
      ambient_r = 0.5f * ambient_light_level;
      ambient_g = 0.5f * ambient_light_level;
      ambient_b = 0.5f * ambient_light_level;
    }

    glClearColor(ambient_r, ambient_g, ambient_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!gpu_lights_.empty()) {
      auto* gl_shader = shader->getShader();
      gl_shader->Use();

      // Setup Projection for FBO: Ortho 0..buffer_w, buffer_h..0 (Y-down)
      glm::mat4 fbo_projection = glm::ortho(0.0f, static_cast<float>(buffer_w),
                                            static_cast<float>(buffer_h), 0.0f);
      gl_shader->SetMat4("uProjection", fbo_projection);
      gl_shader->SetFloat("uTileSize", static_cast<float>(TILE_SIZE) / view.zoom);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shader->getSSBO()->GetID());
      glBindVertexArray(shader->getVAO()->GetID());

      // Enable MAX blending
      {
        ScopedGLCapability blend_cap(GL_BLEND);
        ScopedGLBlend blend_state(GL_ONE, GL_ONE, GL_MAX); 

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4,
                              static_cast<GLsizei>(gpu_lights_.size()));
      }

      glBindVertexArray(0);
    }
  }

  // 4. Composite FBO to Screen
  auto* gl_shader = shader->getShader();
  gl_shader->Use();
  gl_shader->SetInt("uMode", 1); // Composite Mode

  // Bind FBO texture
  glBindTextureUnit(0, fbo->getTexture()->GetID());
  gl_shader->SetInt("uTexture", 0);

  // Quad Transform for Screen
  float draw_dest_x = 0.0f;
  float draw_dest_y = 0.0f;
  float draw_dest_w = static_cast<float>(buffer_w) * view.zoom;
  float draw_dest_h = static_cast<float>(buffer_h) * view.zoom;

  glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(draw_dest_x, draw_dest_y, 0.0f));
  model = glm::scale(model, glm::vec3(draw_dest_w, draw_dest_h, 1.0f));
  gl_shader->SetMat4("uProjection", view.projectionMatrix * view.viewMatrix *
                                       model); // reusing uProjection as MVP

  float uv_w = static_cast<float>(buffer_w) / static_cast<float>(fbo->getWidth());
  float uv_h = static_cast<float>(buffer_h) / static_cast<float>(fbo->getHeight());

  gl_shader->SetVec2("uUVMin", glm::vec2(0.0f, uv_h));
  gl_shader->SetVec2("uUVMax", glm::vec2(uv_w, 0.0f));

  // Blending: Dst * Src
  {
    ScopedGLCapability blend_cap(GL_BLEND);
    ScopedGLBlend blend_state(GL_DST_COLOR, GL_ZERO);

    glBindVertexArray(shader->getVAO()->GetID());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
  }

  gl_shader->SetInt("uMode", 0); // Reset
}
