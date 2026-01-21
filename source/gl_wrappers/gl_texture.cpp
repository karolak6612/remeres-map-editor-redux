#include "gl_texture.h"

GLTexture::GLTexture() : id(0) {}

GLTexture::~GLTexture() {
	destroy();
}

GLTexture::GLTexture(GLTexture&& other) noexcept : id(other.id) {
	other.id = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
	if (this != &other) {
		destroy();
		id = other.id;
		other.id = 0;
	}
	return *this;
}

void GLTexture::create() {
	if (id == 0) {
		glGenTextures(1, &id);
	}
}

void GLTexture::destroy() {
	if (id != 0) {
		glDeleteTextures(1, &id);
		id = 0;
	}
}

void GLTexture::bind() const {
	glBindTexture(GL_TEXTURE_2D, id);
}

void GLTexture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}
