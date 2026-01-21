#ifndef RME_GL_TEXTURE_H
#define RME_GL_TEXTURE_H

#include "../main.h"

class GLTexture {
public:
	GLTexture();
	~GLTexture();

	// Delete copy constructor and assignment
	GLTexture(const GLTexture&) = delete;
	GLTexture& operator=(const GLTexture&) = delete;

	// Move constructor
	GLTexture(GLTexture&& other) noexcept;
	GLTexture& operator=(GLTexture&& other) noexcept;

	void create();
	void destroy();

	void bind() const;
	void unbind() const;

	GLuint getID() const {
		return id;
	}
	bool isValid() const {
		return id != 0;
	}

private:
	GLuint id = 0;
};

#endif
