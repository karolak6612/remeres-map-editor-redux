#ifndef RME_RENDERING_CORE_LIGHT_SHADER_H_
#define RME_RENDERING_CORE_LIGHT_SHADER_H_

#include "rendering/core/gl_resources.h"
#include "rendering/core/shader_program.h"
#include <span>
#include <memory>
#include <glm/glm.hpp>

struct GPULight {
  glm::vec2 position; // 8 bytes (offset 0)
  float intensity;    // 4 bytes (offset 8)
  float padding;      // 4 bytes (offset 12) -> Aligns color to 16 bytes
  glm::vec4 color;    // 16 bytes (offset 16) -> Total 32 bytes
};

static_assert(sizeof(GPULight) == 32, "GPULight must be 32 bytes for std430");

class LightShader {
public:
  LightShader();
  ~LightShader();

  // Upload light data to the SSBO. Re-allocates if capacity is exceeded.
  void Upload(std::span<const GPULight> lights);

  // Getters for rendering
  ShaderProgram* getShader() const { return shader.get(); }
  GLVertexArray* getVAO() const { return vao.get(); }
  GLBuffer* getSSBO() const { return light_ssbo.get(); }

private:
  std::unique_ptr<ShaderProgram> shader;
  std::unique_ptr<GLVertexArray> vao;
  std::unique_ptr<GLBuffer> vbo;
  std::unique_ptr<GLBuffer> light_ssbo;
  size_t light_ssbo_capacity_ = 0; // Track capacity in bytes
};

#endif
