#ifndef RME_RENDERING_LIGHTING_LIGHT_MAP_GENERATOR_H_
#define RME_RENDERING_LIGHTING_LIGHT_MAP_GENERATOR_H_

#include "rendering/core/gl_resources.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/shader_program.h"
#include "rendering/core/render_view.h"
#include "rendering/core/ring_buffer.h"
#include <vector>
#include <memory>

struct GPULightInstance;

class LightMapGenerator {
public:
	LightMapGenerator();
	~LightMapGenerator();

	bool initialize();

	// Renders lights into an FBO texture.
	// Returns the texture ID of the generated light map.
	// ambient_light: 0.0 (dark) to 1.0 (bright)
	GLuint generate(const RenderView& view, const std::vector<LightBuffer::Light>& lights, float ambient_light);

private:
	bool resizeFBO(int width, int height);
	GLuint getAmbientTexture(float ambient_light);
	void prepareInstances(const std::vector<LightBuffer::Light>& lights, const RenderView& view, std::vector<GPULightInstance>& instances);

	std::unique_ptr<GLFramebuffer> fbo;
	std::unique_ptr<GLTextureResource> texture;
	std::unique_ptr<ShaderProgram> shader;
	std::unique_ptr<GLVertexArray> vao;

	// Replaced VBO with RingBuffer for streaming
	RingBuffer ring_buffer;

	std::unique_ptr<GLTextureResource> ambient_tex;
	float last_ambient = -1.0f;

	int width = 0;
	int height = 0;
};

#endif
