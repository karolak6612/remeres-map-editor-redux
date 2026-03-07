#include "rendering/core/light_shader.h"
#include <spdlog/spdlog.h>
#include <algorithm>

LightShader::LightShader() {
  const char *vs = R"(
		#version 450 core
		layout (location = 0) in vec2 aPos; // 0..1 Quad
		
		uniform int uMode;
		uniform mat4 uProjection;
		uniform float uTileSize;
		uniform vec2 uUVMin;
		uniform vec2 uUVMax;

		struct Light {
			vec2 position; 
			float intensity;
			float padding;
			vec4 color;
		};
		layout(std430, binding = 0) buffer LightBlock {
			Light uLights[];
		};

		out vec2 TexCoord;
		out vec4 FragColor; // For Mode 0

		void main() {
			if (uMode == 0) {
				// LIGHT GENERATION
				Light l = uLights[gl_InstanceID];
				
				// Calculate quad size/pos in FBO space
				// Radius spans 0..1 in distance math
				// Quad size should cover the light radius
				// light.intensity is in 'tiles'. 
				// The falloff is 1.0 at center, 0.0 at radius = intensity * TILE_SIZE.
				float radiusPx = l.intensity * uTileSize;
				float size = radiusPx * 2.0;
				
				// Center position
				vec2 center = l.position;
				
				// Vertex Pos (0..1) -> Local Pos (-size/2 .. +size/2) -> World Pos
				vec2 localPos = (aPos - 0.5) * size;
				vec2 worldPos = center + localPos;
				
				gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
				
				// Pass data to fragment
				TexCoord = aPos - 0.5; // -0.5 to 0.5
				FragColor = l.color;
			} else {
				// COMPOSITE
				gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
				TexCoord = mix(uUVMin, uUVMax, aPos); 
			}
		}
	)";

  const char *fs = R"(
		#version 450 core
		in vec2 TexCoord;
		in vec4 FragColor; // From VS
		
		uniform int uMode;
		uniform sampler2D uTexture;

		out vec4 OutColor;

		void main() {
			if (uMode == 0) {
				// Light Falloff
				// TexCoord is -0.5 to 0.5
				float dist = length(TexCoord) * 2.0; // 0.0 to 1.0 (at edge of quad)
				if (dist > 1.0) discard;
				
				float falloff = 1.0 - dist;
				OutColor = FragColor * falloff;
			} else {
				// Texture fetch
				OutColor = texture(uTexture, TexCoord);
			}
		}
	)";

  shader = std::make_unique<ShaderProgram>();
  if (!shader->Load(vs, fs)) {
    spdlog::error("LightShader: Failed to compile light shader");
    shader.reset();
    return;
  }

  float vertices[] = {
      0.0f, 0.0f, // BL
      1.0f, 0.0f, // BR
      1.0f, 1.0f, // TR
      0.0f, 1.0f  // TL
  };

  vao = std::make_unique<GLVertexArray>();
  vbo = std::make_unique<GLBuffer>();
  light_ssbo = std::make_unique<GLBuffer>();

  glNamedBufferStorage(vbo->GetID(), sizeof(vertices), vertices, 0);

  glVertexArrayVertexBuffer(vao->GetID(), 0, vbo->GetID(), 0,
                            2 * sizeof(float));

  glEnableVertexArrayAttrib(vao->GetID(), 0);
  glVertexArrayAttribFormat(vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(vao->GetID(), 0, 0);

  glBindVertexArray(0);
}

LightShader::~LightShader() = default;

void LightShader::Upload(std::span<const GPULight> lights) {
  if (lights.empty()) {
    return;
  }

  size_t needed_size = lights.size() * sizeof(GPULight);
  if (needed_size > light_ssbo_capacity_) {
    light_ssbo_capacity_ = std::max(
        needed_size, static_cast<size_t>(light_ssbo_capacity_ * 1.5));
    if (light_ssbo_capacity_ < 1024) {
      light_ssbo_capacity_ = 1024;
    }
    // Destroy and recreate buffer for Immutable Storage
    light_ssbo = std::make_unique<GLBuffer>();
    glNamedBufferStorage(light_ssbo->GetID(), light_ssbo_capacity_, nullptr,
                         GL_DYNAMIC_STORAGE_BIT);
  }
  glNamedBufferSubData(light_ssbo->GetID(), 0, needed_size, lights.data());
}
