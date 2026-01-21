#ifndef RME_GL_BUFFER_H
#define RME_GL_BUFFER_H

#include "../main.h"

enum class BufferType {
	Vertex,
	Index
};

class GLBuffer {
public:
	explicit GLBuffer(BufferType type);
	~GLBuffer();

	GLBuffer(const GLBuffer&) = delete;
	GLBuffer& operator=(const GLBuffer&) = delete;

	GLBuffer(GLBuffer&& other) noexcept;
	GLBuffer& operator=(GLBuffer&& other) noexcept;

	void create();
	void destroy();

	void bind() const;
	void unbind() const;

	void setData(const void* data, size_t size, GLenum usage);

	GLuint getID() const {
		return id;
	}

private:
	GLuint id = 0;
	GLenum target;
};

#endif
