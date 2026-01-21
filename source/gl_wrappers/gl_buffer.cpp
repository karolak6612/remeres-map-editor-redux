#include "gl_buffer.h"

GLBuffer::GLBuffer(BufferType type) : id(0) {
	switch (type) {
		case BufferType::Vertex:
			target = GL_ARRAY_BUFFER;
			break;
		case BufferType::Index:
			target = GL_ELEMENT_ARRAY_BUFFER;
			break;
	}
}

GLBuffer::~GLBuffer() {
	destroy();
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept : id(other.id), target(other.target) {
	other.id = 0;
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept {
	if (this != &other) {
		destroy();
		id = other.id;
		target = other.target;
		other.id = 0;
	}
	return *this;
}

void GLBuffer::create() {
	if (id == 0) {
		glGenBuffers(1, &id);
	}
}

void GLBuffer::destroy() {
	if (id != 0) {
		glDeleteBuffers(1, &id);
		id = 0;
	}
}

void GLBuffer::bind() const {
	glBindBuffer(target, id);
}

void GLBuffer::unbind() const {
	glBindBuffer(target, 0);
}

void GLBuffer::setData(const void* data, size_t size, GLenum usage) {
	bind();
	glBufferData(target, size, data, usage);
}
