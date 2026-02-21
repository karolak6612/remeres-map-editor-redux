#include "rendering/lighting/light_map_generator.h"
#include "rendering/core/shared_geometry.h"
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include "app/definitions.h"
#include <bit> // C++20 for bit_cast

// Shader to render lights as additive sprites onto the lightmap
const char* light_vert = R"(
#version 460 core
layout (location = 0) in vec2 aPos; // Quad vertices
layout (location = 1) in vec2 aTexCoord; // Quad UVs
layout (location = 2) in vec2 aLightPos; // Instance Pos
layout (location = 3) in vec2 aLightParams; // Instance Color (packed), Intensity

out vec2 TexCoord;
out vec4 LightColor;
out float Intensity;

uniform mat4 uMVP;

vec4 unpackColor(float f) {
    uint u = floatBitsToUint(f);
    return vec4(
        float((u >> 0) & 0xFF) / 255.0,
        float((u >> 8) & 0xFF) / 255.0,
        float((u >> 16) & 0xFF) / 255.0,
        1.0
    );
}

void main() {
    // Light sprite size depends on intensity.
    float size = aLightParams.y * 64.0; // Arbitrary scale factor

    vec2 pos = aLightPos + (aPos - 0.5) * size; // Center quad on position
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
    LightColor = unpackColor(aLightParams.x);
    Intensity = aLightParams.y;
}
)";

const char* light_frag = R"(
#version 460 core
in vec2 TexCoord;
in vec4 LightColor;
in float Intensity;
out vec4 FragColor;

void main() {
    // Simple radial falloff
    float dist = length(TexCoord - 0.5) * 2.0;
    if (dist > 1.0) discard;

    float alpha = (1.0 - dist) * (1.0 - dist); // Quadratic falloff

    // Additive blend: SRC_ALPHA, ONE. Output must be pre-multiplied by alpha logic?
    // Review: "change the output so RGB remains the unmultiplied LightColor and only the alpha carries the falloff value"
    // FragColor = vec4(LightColor.rgb, alpha);

    FragColor = vec4(LightColor.rgb, alpha);
}
)";

LightMapGenerator::LightMapGenerator() {
}

LightMapGenerator::~LightMapGenerator() {
}

bool LightMapGenerator::initialize() {
	shader = std::make_unique<ShaderProgram>();
	if (!shader->Load(light_vert, light_frag)) {
		spdlog::error("LightMapGenerator: Failed to load shader");
		return false;
	}

	if (!SharedGeometry::Instance().initialize()) {
		return false;
	}

	vao = std::make_unique<GLVertexArray>();
	vbo = std::make_unique<GLBuffer>();

	// Setup Quad VBO (Binding 0)
	glVertexArrayVertexBuffer(vao->GetID(), 0, SharedGeometry::Instance().getQuadVBO(), 0, 4 * sizeof(float));
	glVertexArrayElementBuffer(vao->GetID(), SharedGeometry::Instance().getQuadEBO());

	// Loc 0: position (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 0);
	glVertexArrayAttribFormat(vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao->GetID(), 0, 0);

	// Loc 1: texcoord (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 1);
	glVertexArrayAttribFormat(vao->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao->GetID(), 1, 0);

	// Instance Buffer (Binding 1)
	glVertexArrayBindingDivisor(vao->GetID(), 1, 1);

	// Loc 2: LightPos (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 2);
	glVertexArrayAttribFormat(vao->GetID(), 2, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao->GetID(), 2, 1);

	// Loc 3: LightParams (vec2)
	glEnableVertexArrayAttrib(vao->GetID(), 3);
	glVertexArrayAttribFormat(vao->GetID(), 3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao->GetID(), 3, 1);

	return true;
}

void LightMapGenerator::resizeFBO(int w, int h) {
	if (width != w || height != h || !fbo) {
		width = w;
		height = h;
		fbo = std::make_unique<GLFramebuffer>();
		texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

		glTextureStorage2D(texture->GetID(), 1, GL_RGBA8, width, height);
		glTextureParameteri(texture->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture->GetID(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture->GetID(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glNamedFramebufferTexture(fbo->GetID(), GL_COLOR_ATTACHMENT0, texture->GetID(), 0);

		GLenum status = glCheckNamedFramebufferStatus(fbo->GetID(), GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			spdlog::error("LightMapGenerator FBO incomplete: status={}", status);
		}
	}
}

GLuint LightMapGenerator::generate(const RenderView& view, const std::vector<LightBuffer::Light>& lights, float ambient_light) {
	if (lights.empty()) {
		// Return 1x1 texture with ambient color
		static std::unique_ptr<GLTextureResource> ambient_tex;
		static float last_ambient = -1.0f;

		if (!ambient_tex) {
			ambient_tex = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
			glTextureStorage2D(ambient_tex->GetID(), 1, GL_RGBA8, 1, 1);
			glTextureParameteri(ambient_tex->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(ambient_tex->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(ambient_tex->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(ambient_tex->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		if (last_ambient != ambient_light) {
			uint8_t val = static_cast<uint8_t>(ambient_light * 255.0f);
			uint8_t pixel[4] = {val, val, val, 255};
			glTextureSubImage2D(ambient_tex->GetID(), 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
			last_ambient = ambient_light;
		}
		return ambient_tex->GetID();
	}

	int target_w = view.screensize_x;
	int target_h = view.screensize_y;
	resizeFBO(target_w, target_h);

	// Save GL state
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	GLint prevFBO;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFBO);

	// Save Clear Color
	GLfloat prevClearColor[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClearColor);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->GetID());
	glViewport(0, 0, target_w, target_h);

	// Clear with ambient color
	glClearColor(ambient_light, ambient_light, ambient_light, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Upload lights
	struct GPULightInstance {
		float x, y;
		float packed_color; // uint as float
		float intensity;
	};

	std::vector<GPULightInstance> instances;
	instances.reserve(lights.size());

	for (const auto& l : lights) {
		// Map coordinate to screen coordinate.
		// The collector applies z-offsets to map_x/map_y, so we treat them as 2D plane coords.
		float screen_x = l.map_x * TILE_SIZE - view.view_scroll_x;
		float screen_y = l.map_y * TILE_SIZE - view.view_scroll_y;

		// Resolve the color from the 8-bit index before packing
		wxColor color = colorFromEightBit(l.color);
		// Fix shift: 0xFFu
		uint32_t packed_val = (color.Red()) | (color.Green() << 8) | (color.Blue() << 16) | (0xFFu << 24);

		float packed_color_f = std::bit_cast<float>(packed_val);

		instances.push_back({
			screen_x + TILE_SIZE / 2.0f, // Center of tile
			screen_y + TILE_SIZE / 2.0f,
			packed_color_f,
			static_cast<float>(l.intensity) / 255.0f // Normalized intensity
		});
	}

	glNamedBufferData(vbo->GetID(), instances.size() * sizeof(GPULightInstance), instances.data(), GL_STREAM_DRAW);

	// Draw
	// Save Blend state
	GLboolean blend_enabled = glIsEnabled(GL_BLEND);
	GLint blend_src_rgb, blend_dst_rgb, blend_src_alpha, blend_dst_alpha;
	glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_rgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_alpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_alpha);

	shader->Use();
	shader->SetMat4("uMVP", view.projectionMatrix);

	glBindVertexArray(vao->GetID());
	glVertexArrayVertexBuffer(vao->GetID(), 1, vbo->GetID(), 0, sizeof(GPULightInstance));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending

	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instances.size()));

	// Restore GL state
	glBlendFuncSeparate(blend_src_rgb, blend_dst_rgb, blend_src_alpha, blend_dst_alpha);
	if (!blend_enabled) glDisable(GL_BLEND);
	shader->Unuse();

	// Restore FBO and Viewport
	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	// Restore Clear Color
	glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);

	return texture->GetID();
}
