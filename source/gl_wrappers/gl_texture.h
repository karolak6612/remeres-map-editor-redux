#ifndef RME_GL_TEXTURE_H
#define RME_GL_TEXTURE_H

#include "../main.h"

#if defined(__LINUX__) || defined(__WINDOWS__)
	#include <GL/glut.h>
#endif

class GLTexture {
public:
	GLTexture();
	~GLTexture();

	// Delete copy
	GLTexture(const GLTexture&) = delete;
	GLTexture& operator=(const GLTexture&) = delete;

	// Allow move
	GLTexture(GLTexture&& other) noexcept;
	GLTexture& operator=(GLTexture&& other) noexcept;

	void generate();
	void bind() const;
	void unbind() const;
	void upload(int width, int height, const void* data, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);

	// Wraps parameters
	void setFilter(GLenum minFilter, GLenum magFilter);
	void setWrap(GLenum wrapS, GLenum wrapT);

	GLuint getID() const {
		return id;
	}

private:
	GLuint id = 0;
};

#endif
