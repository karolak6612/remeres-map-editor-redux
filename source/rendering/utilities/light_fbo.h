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

#ifndef RME_LIGHT_FBO_H
#define RME_LIGHT_FBO_H

#include <memory>
#include "rendering/core/gl_resources.h"

// Owns the light framebuffer and its attached texture.
// Handles creation, resize, and provides access to GL IDs.
class LightFBO {
public:
	LightFBO();

	// Ensures the FBO texture is at least (width x height).
	// Recreates the texture if the current size is insufficient.
	void EnsureSize(int width, int height);

	GLuint GetFBOID() const { return fbo_->GetID(); }
	GLuint GetTextureID() const { return texture_->GetID(); }
	int GetWidth() const { return width_; }
	int GetHeight() const { return height_; }

private:
	std::unique_ptr<GLFramebuffer> fbo_;
	std::unique_ptr<GLTextureResource> texture_;
	int width_ = 0;
	int height_ = 0;

	void createTexture(int width, int height);
};

#endif
