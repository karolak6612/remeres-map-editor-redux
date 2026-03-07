#ifndef RME_RENDERING_CORE_LIGHT_FBO_H_
#define RME_RENDERING_CORE_LIGHT_FBO_H_

#include "rendering/core/gl_resources.h"
#include <memory>

class LightFBO {
public:
  LightFBO();
  ~LightFBO();

  // Resize the framebuffer and its backing texture if needed
  void Resize(int width, int height);

  // Getters for rendering
  GLFramebuffer* getFBO() const { return fbo.get(); }
  GLTextureResource* getTexture() const { return fbo_texture.get(); }

  int getWidth() const { return buffer_width; }
  int getHeight() const { return buffer_height; }

private:
  std::unique_ptr<GLFramebuffer> fbo;
  std::unique_ptr<GLTextureResource> fbo_texture;
  int buffer_width = 0;
  int buffer_height = 0;

  void InitFBOStorage(int width, int height);
};

#endif
