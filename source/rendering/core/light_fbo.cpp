#include "rendering/core/light_fbo.h"
#include <algorithm>
#include <spdlog/spdlog.h>

LightFBO::LightFBO() {
  fbo = std::make_unique<GLFramebuffer>();
  fbo_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

  // Initial dummy size
  InitFBOStorage(32, 32);

  glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0,
                            fbo_texture->GetID(), 0);

  GLenum status = glCheckNamedFramebufferStatus(fbo->GetID(), GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("LightFBO Incomplete: {}", status);
    fbo.reset();
    fbo_texture.reset();
    return;
  }
}

LightFBO::~LightFBO() = default;

void LightFBO::InitFBOStorage(int width, int height) {
  if (width == buffer_width && height == buffer_height) {
    return;
  }

  buffer_width = width;
  buffer_height = height;

  glTextureStorage2D(fbo_texture->GetID(), 1, GL_RGBA8, width, height);

  // Set texture parameters
  glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_WRAP_S,
                      GL_CLAMP_TO_EDGE);
  glTextureParameteri(fbo_texture->GetID(), GL_TEXTURE_WRAP_T,
                      GL_CLAMP_TO_EDGE);
}

void LightFBO::Resize(int width, int height) {
  if (buffer_width >= width && buffer_height >= height) {
    return;
  }

  fbo_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
  InitFBOStorage(std::max(buffer_width, width),
                 std::max(buffer_height, height));
  
  if (fbo) {
    glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0,
                              fbo_texture->GetID(), 0);
  }
}
