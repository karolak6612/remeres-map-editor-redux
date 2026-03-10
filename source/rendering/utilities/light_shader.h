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

#ifndef RME_LIGHT_SHADER_H
#define RME_LIGHT_SHADER_H

#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "rendering/core/shader_program.h"
#include "rendering/core/gl_resources.h"

struct GPULight;

// Owns the light shader program, VAO/VBO geometry, and SSBO.
// Provides methods to upload light data and issue draw commands.
class LightShader {
public:
	LightShader();

	// Upload GPU light data to the SSBO. Must be called before DrawLightPass.
	void Upload(std::span<const GPULight> lights);

	// Draw instanced light quads in Mode 0 (light generation).
	// Requires an active FBO binding and viewport.
	void DrawLightPass(const glm::mat4& projection, float tileSize, int lightCount);

	// Draw the composite quad in Mode 1 (texture blit).
	void DrawComposite(const glm::mat4& mvp, GLuint textureID, glm::vec2 uvMin, glm::vec2 uvMax);

private:
	std::unique_ptr<ShaderProgram> shader_;
	std::unique_ptr<GLVertexArray> vao_;
	std::unique_ptr<GLBuffer> vbo_;
	std::unique_ptr<GLBuffer> ssbo_;
	size_t ssbo_capacity_ = 0;

	void init();
};

#endif
