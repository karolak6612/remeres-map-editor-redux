#include "gl_texture.h"

GLTexture::GLTexture() :
	id(0) {
}

GLTexture::~GLTexture() {
	if (id != 0) {
		glDeleteTextures(1, &id);
	}
}

GLTexture::GLTexture(GLTexture&& other) noexcept :
	id(other.id) {
	other.id = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
	if (this != &other) {
		if (id != 0) {
			glDeleteTextures(1, &id);
		}
		id = other.id;
		other.id = 0;
	}
	return *this;
}

void GLTexture::generate() {
	if (id == 0) {
		glGenTextures(1, &id);
	}
}

void GLTexture::bind() const {
	if (id != 0) {
		glBindTexture(GL_TEXTURE_2D, id);
	}
}

void GLTexture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::upload(int width, int height, const void* data, GLenum format, GLenum type) {
	if (id == 0) {
		generate();
	}
	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, type, data);
}

void GLTexture::setFilter(GLenum minFilter, GLenum magFilter) {
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void GLTexture::setWrap(GLenum wrapS, GLenum wrapT) {
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}
