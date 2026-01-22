#include "gl_renderer.h"

// Ensure prototypes
#if defined(__LINUX__)
	#ifndef GL_GLEXT_PROTOTYPES
	#define GL_GLEXT_PROTOTYPES
	#endif
	#include <GL/gl.h>
	#include <GL/glext.h>
#endif

BatchRenderer::BatchRenderer() :
	vbo(GLBuffer::VERTEX),
	ibo(GLBuffer::INDEX) {

	vertices.reserve(MAX_VERTICES);

	// Initialize IBO
	std::vector<uint32_t> indices;
	indices.reserve(MAX_INDICES);
	for (size_t i = 0; i < MAX_QUADS; ++i) {
		uint32_t offset = i * 4;
		indices.push_back(offset + 0);
		indices.push_back(offset + 1);
		indices.push_back(offset + 2);
		indices.push_back(offset + 2);
		indices.push_back(offset + 3);
		indices.push_back(offset + 0);
	}
	ibo.setData(indices.data(), indices.size() * sizeof(uint32_t), GL_STATIC_DRAW);
}

BatchRenderer::~BatchRenderer() {
}

void BatchRenderer::begin() {
	vertices.clear();
	currentTexture = 0;
}

void BatchRenderer::end() {
	flush();
}

void BatchRenderer::flush() {
	if (vertices.empty()) {
		return;
	}

	if (currentTexture != 0) {
		glBindTexture(GL_TEXTURE_2D, currentTexture);
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_TEXTURE_2D);
	}

	vbo.setData(vertices.data(), vertices.size() * sizeof(Vertex), GL_DYNAMIC_DRAW);

	vbo.bind();
	ibo.bind();

	// Enable attributes
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(2, GL_FLOAT, sizeof(Vertex), (const void*)offsetof(Vertex, x));
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (const void*)offsetof(Vertex, u));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (const void*)offsetof(Vertex, r));

	glDrawElements(GL_TRIANGLES, (vertices.size() / 4) * 6, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	vbo.unbind();
	ibo.unbind();

	vertices.clear();
}

void BatchRenderer::drawQuad(float x, float y, float w, float h, GLuint texture, float u1, float v1, float u2, float v2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	if (currentTexture != texture) {
		flush();
		currentTexture = texture;
	}

	if (vertices.size() >= MAX_VERTICES) {
		flush();
	}

	// BL, BR, TR, TL (Standard Quad winding logic adapted to Top-Left origin)
	// Vertices order in buffer: TL, TR, BR, BL (to match IBO 0,1,2, 2,3,0)

	vertices.push_back({ x, y, u1, v1, r, g, b, a }); // TL
	vertices.push_back({ x + w, y, u2, v1, r, g, b, a }); // TR
	vertices.push_back({ x + w, y + h, u2, v2, r, g, b, a }); // BR
	vertices.push_back({ x, y + h, u1, v2, r, g, b, a }); // BL
}

void BatchRenderer::drawRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	drawQuad(x, y, w, h, 0, 0, 0, 0, 0, r, g, b, a);
}
