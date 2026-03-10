#ifndef RME_RENDERING_CORE_SHARED_GEOMETRY_H_
#define RME_RENDERING_CORE_SHARED_GEOMETRY_H_

#include "rendering/core/gl_resources.h"
#include <memory>
#include <unordered_map>
#include <mutex>

class SharedGeometry {
public:
	SharedGeometry() = default;
	~SharedGeometry() = default;

	SharedGeometry(const SharedGeometry&) = delete;
	SharedGeometry& operator=(const SharedGeometry&) = delete;

	bool initialize();
	GLuint getQuadVBO();
	GLuint getQuadEBO();

	// Returns number of indices for the quad (6)
	GLsizei getQuadIndexCount() const {
		return 6;
	}

private:
	struct ContextGeometry {
		std::unique_ptr<GLBuffer> vbo;
		std::unique_ptr<GLBuffer> ebo;
	};

	std::unordered_map<void*, ContextGeometry> contexts_;
	std::mutex mutex_;
};

#endif
