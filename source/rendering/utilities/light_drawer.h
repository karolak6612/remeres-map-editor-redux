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

class LightDrawer {
public:
	LightDrawer();
	~LightDrawer();

	void draw(const RenderView& view, const LightBuffer& light_buffer, const DrawingOptions& options);

	// Call when the map changes (tiles added/removed, light sources change)
	void markDirty();

private:
	void computeBrightness(const LightBuffer& light_buffer, const DrawingOptions& options);
	void initRenderResources();

	std::unique_ptr<ShaderProgram> shader_;
	std::unique_ptr<GLVertexArray> vao_;
	std::unique_ptr<GLBuffer> vbo_;
	std::unique_ptr<GLTextureResource> light_texture_;
	std::vector<uint8_t> tile_brightness_;
	std::vector<uint8_t> screen_pixels_;
	int tex_width_ = 0;
	int tex_height_ = 0;
	int cached_tw_ = 0;
	int cached_th_ = 0;
	bool dirty_ = true;

	// Track lighting params to detect changes
	float cached_ambient_ = -1.0f;
	uint8_t cached_server_intensity_ = 0;
	uint8_t cached_server_color_ = 0;
	bool cached_show_lights_ = false;
	size_t cached_light_count_ = 0;
	size_t cached_light_hash_ = 0;
};

#endif
