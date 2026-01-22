#ifndef RME_GL_RENDERER_H
#define RME_GL_RENDERER_H

#include "../main.h"
#include "gl_buffer.h"
#include "gl_texture.h"
#include <vector>

struct Vertex {
	float x, y;
	float u, v;
	uint8_t r, g, b, a;
};

class BatchRenderer {
public:
	BatchRenderer();
	~BatchRenderer();

	void begin();
	void end();
	void flush();

	void drawQuad(float x, float y, float w, float h, GLuint texture, float u1, float v1, float u2, float v2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

private:
	static const size_t MAX_QUADS = 10000;
	static const size_t MAX_VERTICES = MAX_QUADS * 4;
	static const size_t MAX_INDICES = MAX_QUADS * 6;

	std::vector<Vertex> vertices;
	GLuint currentTexture = 0;

	GLBuffer vbo;
	GLBuffer ibo;
};

#endif
