#include "rendering/core/shared_geometry.h"
#include <spdlog/spdlog.h>
#ifdef _WIN32
	#include <windows.h>
#elif defined(__linux__)
	#include <GL/glx.h>
#elif defined(__APPLE__)
	#include <OpenGL/OpenGL.h>
#endif

void* GetCurrentGLContext() {
#ifdef _WIN32
	return (void*)w// gl API removed
#elif defined(__linux__)
	return (void*)// gl API removed
#elif defined(__APPLE__)
	return (void*)CGLGetCurrentContext();
#else
	return nullptr;
#endif
}

bool SharedGeometry::initialize() {
	void* ctx = GetCurrentGLContext();
	if (!ctx) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);

	if (auto it = contexts_.find(ctx); it != contexts_.end()) {
		return true;
	}

	auto& geom = contexts_[ctx];
	geom.vbo = std::make_unique<GLBuffer>();
	geom.ebo = std::make_unique<GLBuffer>();

	// Unit quad geometry (0..1)
	// Pos (vec2), Tex (vec2)
	// Composed of 4 vertices
	float quad_vertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f, // pos.xy, tex.xy
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};

	unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

	// Upload static geometry (Immutable storage)
	// gl API removed
	// gl API removed

	spdlog::info("SharedGeometry: initialized for context {} (VBO: {}, EBO: {})", ctx, geom.vbo->GetID(), geom.ebo->GetID());
	return true;
}

uint32_t SharedGeometry::getQuadVBO() {
	initialize();
	void* ctx = GetCurrentGLContext();
	std::lock_guard<std::mutex> lock(mutex_);
	if (auto it = contexts_.find(ctx); it != contexts_.end()) {
		return it->second.vbo->GetID();
	}
	return 0;
}

uint32_t SharedGeometry::getQuadEBO() {
	initialize();
	void* ctx = GetCurrentGLContext();
	std::lock_guard<std::mutex> lock(mutex_);
	if (auto it = contexts_.find(ctx); it != contexts_.end()) {
		return it->second.ebo->GetID();
	}
	return 0;
}
