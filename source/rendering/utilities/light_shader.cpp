//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/utilities/light_shader.h"
#include "rendering/utilities/light_drawer.h" // For GPULight
#include "rendering/core/gl_scoped_state.h"
#include <limits>

LightShader::LightShader() {
	init();
}

void LightShader::Upload(std::span<const GPULight> lights) {
	if (lights.empty()) {
		return;
	}

	size_t needed_size = lights.size() * sizeof(GPULight);
	if (needed_size > ssbo_capacity_) {
		ssbo_capacity_ = std::max(needed_size, static_cast<size_t>(ssbo_capacity_ * 1.5));
		if (ssbo_capacity_ < 1024) {
			ssbo_capacity_ = 1024;
		}
		// Destroy and recreate buffer for Immutable Storage
		ssbo_ = std::make_unique<GLBuffer>();
		glNamedBufferStorage(ssbo_->GetID(), ssbo_capacity_, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}
	glNamedBufferSubData(ssbo_->GetID(), 0, needed_size, lights.data());
}

void LightShader::DrawLightPass(const glm::mat4& projection, float tileSize, int lightCount) {
	shader_->Use();
	shader_->SetInt("uMode", 0);
	shader_->SetMat4("uProjection", projection);
	shader_->SetFloat("uTileSize", tileSize);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_->GetID());
	glBindVertexArray(vao_->GetID());

	{
		ScopedGLCapability blendCap(GL_BLEND);
		ScopedGLBlend blendState(GL_ONE, GL_ONE, GL_MAX);

		if (lightCount > std::numeric_limits<GLsizei>::max()) {
			spdlog::error("Too many lights for glDrawArraysInstanced");
			return;
		}
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(lightCount));
	}

	glBindVertexArray(0);
}

void LightShader::DrawComposite(const glm::mat4& mvp, GLuint textureID, glm::vec2 uvMin, glm::vec2 uvMax) {
	shader_->Use();
	shader_->SetInt("uMode", 1);

	glBindTextureUnit(0, textureID);
	shader_->SetInt("uTexture", 0);
	shader_->SetMat4("uProjection", mvp);
	shader_->SetVec2("uUVMin", uvMin);
	shader_->SetVec2("uUVMax", uvMax);

	{
		ScopedGLCapability blendCap(GL_BLEND);
		ScopedGLBlend blendState(GL_DST_COLOR, GL_ZERO);

		glBindVertexArray(vao_->GetID());
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}
}

void LightShader::init() {
	const char* vs = R"(
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

				float radiusPx = l.intensity * uTileSize;
				float size = radiusPx * 2.0;

				vec2 center = l.position;
				vec2 localPos = (aPos - 0.5) * size;
				vec2 worldPos = center + localPos;

				gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);

				TexCoord = aPos - 0.5; // -0.5 to 0.5
				FragColor = l.color;
			} else {
				// COMPOSITE
				gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
				TexCoord = mix(uUVMin, uUVMax, aPos);
			}
		}
	)";

	const char* fs = R"(
		#version 450 core
		in vec2 TexCoord;
		in vec4 FragColor; // From VS

		uniform int uMode;
		uniform sampler2D uTexture;

		out vec4 OutColor;

		void main() {
			if (uMode == 0) {
				// Light Falloff
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

	shader_ = std::make_unique<ShaderProgram>();
	shader_->Load(vs, fs);

	float vertices[] = {
		0.0f, 0.0f, // BL
		1.0f, 0.0f, // BR
		1.0f, 1.0f, // TR
		0.0f, 1.0f // TL
	};

	vao_ = std::make_unique<GLVertexArray>();
	vbo_ = std::make_unique<GLBuffer>();
	ssbo_ = std::make_unique<GLBuffer>();

	glNamedBufferStorage(vbo_->GetID(), sizeof(vertices), vertices, 0);

	glVertexArrayVertexBuffer(vao_->GetID(), 0, vbo_->GetID(), 0, 2 * sizeof(float));

	glEnableVertexArrayAttrib(vao_->GetID(), 0);
	glVertexArrayAttribFormat(vao_->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_->GetID(), 0, 0);

	glBindVertexArray(0);
}
