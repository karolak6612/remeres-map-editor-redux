#ifndef RME_LIGHDRAWER_H
#define RME_LIGHDRAWER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "rendering/core/gl_resources.h"
#include "rendering/core/light_buffer.h"
#include "rendering/core/shader_program.h"

struct DrawingOptions;
struct RenderView;

struct GPULight {
	glm::vec2 tile_position;
	float intensity;
	float padding;
	glm::vec4 color;
};

struct GPUTileLight {
	uint32_t start = 0;
	uint32_t color = 0;
};

class LightDrawer {
public:
	LightDrawer();
	~LightDrawer();

	void draw(const RenderView& view, const LightBuffer& light_buffer, const DrawingOptions& options);

private:
	void initFBO();
	void resizeFBO(int width, int height);
	void initRenderResources();
	void uploadBuffer(std::unique_ptr<GLBuffer>& buffer, size_t& capacity, const void* data, size_t count, size_t stride);

	std::unique_ptr<ShaderProgram> shader_;
	std::unique_ptr<GLVertexArray> vao_;
	std::unique_ptr<GLBuffer> vbo_;
	std::unique_ptr<GLBuffer> light_ssbo_;
	std::unique_ptr<GLBuffer> tile_ssbo_;
	size_t light_ssbo_capacity_ = 0;
	size_t tile_ssbo_capacity_ = 0;

	std::vector<GPULight> gpu_lights_;
	std::vector<GPUTileLight> gpu_tiles_;

	std::unique_ptr<GLFramebuffer> fbo_;
	std::unique_ptr<GLTextureResource> fbo_texture_;
	int buffer_width_ = 0;
	int buffer_height_ = 0;
};

#endif
