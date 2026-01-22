#ifndef RME_GL_BUFFER_H
#define RME_GL_BUFFER_H

#include "../main.h"

#if defined(__LINUX__) || defined(__WINDOWS__)
	#include <GL/glut.h>
#endif

// Fallback definitions for older GL headers
#ifndef GL_ARRAY_BUFFER
	#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
	#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
	#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
	#define GL_DYNAMIC_DRAW 0x88E8
#endif

class GLBuffer {
public:
	enum Type {
		VERTEX = GL_ARRAY_BUFFER,
		INDEX = GL_ELEMENT_ARRAY_BUFFER
	};

	GLBuffer(Type type);
	~GLBuffer();

	GLBuffer(const GLBuffer&) = delete;
	GLBuffer& operator=(const GLBuffer&) = delete;

	GLBuffer(GLBuffer&& other) noexcept;
	GLBuffer& operator=(GLBuffer&& other) noexcept;

	void generate();
	void bind() const;
	void unbind() const;
	void setData(const void* data, size_t size, GLenum usage = GL_DYNAMIC_DRAW);

	GLuint getID() const {
		return id;
	}

private:
	GLuint id = 0;
	Type type;
};

#endif
