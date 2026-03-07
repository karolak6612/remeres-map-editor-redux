#ifndef RME_RENDERING_UTILITIES_LIGHT_RENDERER_H_
#define RME_RENDERING_UTILITIES_LIGHT_RENDERER_H_

#include "rendering/core/draw_context.h"
#include <memory>
#include <vector>

class LightFBO;
class LightShader;
struct GPULight;

class LightRenderer {
public:
  LightRenderer();
  ~LightRenderer();

  void draw(const DrawContext &ctx);

private:
  std::unique_ptr<LightFBO> fbo;
  std::unique_ptr<LightShader> shader;
  std::vector<GPULight> gpu_lights_;
};

#endif
