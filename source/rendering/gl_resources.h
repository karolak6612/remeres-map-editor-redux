#ifndef RME_RENDERING_GL_RESOURCES_H
#define RME_RENDERING_GL_RESOURCES_H

#include "main.h"

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

// RAII wrapper for OpenGL Textures
class GLTexture {
public:
	GLTexture() :
		id(0) { }

	~GLTexture() {
		release();
	}

	// Disable copy
	GLTexture(const GLTexture&) = delete;
	GLTexture& operator=(const GLTexture&) = delete;

	// Enable move
	GLTexture(GLTexture&& other) noexcept :
		id(other.id) {
		other.id = 0;
	}

	GLTexture& operator=(GLTexture&& other) noexcept {
		if (this != &other) {
			release();
			id = other.id;
			other.id = 0;
		}
		return *this;
	}

	void generate() {
		if (id == 0) {
			glGenTextures(1, &id);
		}
	}

	void release() {
		if (id != 0) {
			glDeleteTextures(1, &id);
			id = 0;
		}
	}

	void bind() const {
		glBindTexture(GL_TEXTURE_2D, id);
	}

	operator GLuint() const {
		return id;
	}

	GLuint get() const {
		return id;
	}

private:
	GLuint id;
};

// RAII wrapper for OpenGL Buffers (VBO, IBO, etc)
class GLBuffer {
public:
	GLBuffer() :
		id(0) { }

	~GLBuffer() {
		release();
	}

	// Disable copy
	GLBuffer(const GLBuffer&) = delete;
	GLBuffer& operator=(const GLBuffer&) = delete;

	// Enable move
	GLBuffer(GLBuffer&& other) noexcept :
		id(other.id) {
		other.id = 0;
	}

	GLBuffer& operator=(GLBuffer&& other) noexcept {
		if (this != &other) {
			release();
			id = other.id;
			other.id = 0;
		}
		return *this;
	}

	void generate() {
		if (id == 0) {
			glGenBuffers(1, &id);
		}
	}

	void release() {
		if (id != 0) {
			glDeleteBuffers(1, &id);
			id = 0;
		}
	}

	void bind(GLenum target) const {
		glBindBuffer(target, id);
	}

	operator GLuint() const {
		return id;
	}

	GLuint get() const {
		return id;
	}

private:
	GLuint id;
};

#endif
