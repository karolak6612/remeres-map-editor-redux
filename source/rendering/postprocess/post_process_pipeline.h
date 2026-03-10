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

#ifndef RME_RENDERING_POSTPROCESS_PIPELINE_H_
#define RME_RENDERING_POSTPROCESS_PIPELINE_H_

#include <memory>

class GLFramebuffer;
class GLTextureResource;
class GLVertexArray;
class GLBuffer;
struct ViewState;
struct DrawingOptions;

class PostProcessPipeline {
public:
	PostProcessPipeline() = default;
	~PostProcessPipeline() = default;

	PostProcessPipeline(const PostProcessPipeline&) = delete;
	PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

	void Initialize();

	// Returns true if FBO is active and End() should be called after rendering.
	bool Begin(const ViewState& view, const DrawingOptions& options);

	// Resolves FBO to screen: draws post-process quad, unbinds FBO, restores viewport.
	void End(const ViewState& view, const DrawingOptions& options);

private:
	void UpdateFBO(const ViewState& view, const DrawingOptions& options);
	void DrawPostProcess(const ViewState& view, const DrawingOptions& options);

	std::unique_ptr<GLFramebuffer> scale_fbo_;
	std::unique_ptr<GLTextureResource> scale_texture_;
	int fbo_width_ = 0;
	int fbo_height_ = 0;
	bool last_aa_mode_ = false;

	std::unique_ptr<GLVertexArray> pp_vao_;
	std::unique_ptr<GLBuffer> pp_vbo_;
	std::unique_ptr<GLBuffer> pp_ebo_;
};

#endif
