#ifndef RME_RENDERING_CORE_SHARED_GEOMETRY_H_
#define RME_RENDERING_CORE_SHARED_GEOMETRY_H_

#include "rendering/core/gl_resources.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class SharedGeometry {
public:
  SharedGeometry() = default;
  ~SharedGeometry() = default;

  bool initialize();
  GLuint getQuadVBO();
  GLuint getQuadEBO();

  // Returns number of indices for the quad (6)
  GLsizei getQuadIndexCount() const { return 6; }

  SharedGeometry(const SharedGeometry &) = delete;
  SharedGeometry &operator=(const SharedGeometry &) = delete;
  SharedGeometry(SharedGeometry &&other) noexcept
      : contexts_(std::move(other.contexts_)) {}
  SharedGeometry &operator=(SharedGeometry &&other) noexcept {
    if (this != &other) {
      contexts_ = std::move(other.contexts_);
    }
    return *this;
  }

private:
  struct ContextGeometry {
    std::unique_ptr<GLBuffer> vbo;
    std::unique_ptr<GLBuffer> ebo;
  };

  std::unordered_map<void *, ContextGeometry> contexts_;
  std::mutex mutex_;
};

#endif
