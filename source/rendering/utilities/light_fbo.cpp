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
#include "rendering/utilities/light_fbo.h"

LightFBO::LightFBO() {
	fbo_ = std::make_unique<GLFramebuffer>();
	createTexture(32, 32);

	glNamedFramebufferTexture(fbo_->GetID(), GL_COLOR_ATTACHMENT0, texture_->GetID(), 0);

	GLenum status = glCheckNamedFramebufferStatus(fbo_->GetID(), GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("LightFBO Incomplete: {}", status);
	}
}

void LightFBO::createTexture(int width, int height) {
	width_ = width;
	height_ = height;

	texture_ = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);
	glTextureStorage2D(texture_->GetID(), 1, GL_RGBA8, width, height);

	glTextureParameteri(texture_->GetID(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture_->GetID(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture_->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture_->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void LightFBO::EnsureSize(int width, int height) {
	if (width_ >= width && height_ >= height) {
		return;
	}

	// Grow to at least the requested size (never shrinks)
	createTexture(std::max(width_, width), std::max(height_, height));
	glNamedFramebufferTexture(fbo_->GetID(), GL_COLOR_ATTACHMENT0, texture_->GetID(), 0);

	GLenum status = glCheckNamedFramebufferStatus(fbo_->GetID(), GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("LightFBO Incomplete after resize ({}x{}): {}", width_, height_, status);
	}
}
