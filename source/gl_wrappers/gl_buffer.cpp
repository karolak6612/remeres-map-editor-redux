#include "gl_buffer.h"

#if defined(__LINUX__)
	// Ensure prototypes are defined
	#ifndef GL_GLEXT_PROTOTYPES
		#define GL_GLEXT_PROTOTYPES
	#endif
	#include <GL/gl.h>
	#include <GL/glext.h>
#endif

GLBuffer::GLBuffer(Type type) :
	id(0),
	type(type) {
}

GLBuffer::~GLBuffer() {
	if (id != 0) {
		glDeleteBuffers(1, &id);
	}
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept :
	id(other.id),
	type(other.type) {
	other.id = 0;
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept {
	if (this != &other) {
		if (id != 0) {
			glDeleteBuffers(1, &id);
		}
		id = other.id;
		type = other.type;
		other.id = 0;
	}
	return *this;
}

void GLBuffer::generate() {
	if (id == 0) {
		glGenBuffers(1, &id);
	}
}

void GLBuffer::bind() const {
	if (id != 0) {
		glBindBuffer(type, id);
	}
}

void GLBuffer::unbind() const {
	glBindBuffer(type, 0);
}

void GLBuffer::setData(const void* data, size_t size, GLenum usage) {
	if (id == 0) {
		generate();
	}
	bind();
	glBufferData(type, size, data, usage);
}
