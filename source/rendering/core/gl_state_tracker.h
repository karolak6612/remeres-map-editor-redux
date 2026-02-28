#ifndef RME_RENDERING_CORE_GL_STATE_TRACKER_H_
#define RME_RENDERING_CORE_GL_STATE_TRACKER_H_

#include <glad/glad.h>
#include <array>
#include <memory>
#include <unordered_map>

/**
 * @brief Tracks OpenGL state to avoid redundant calls and expensive queries.
 */
class GLStateTracker {
public:
	static GLStateTracker& Instance();

	// Capabilities (glEnable/glDisable)
	void Enable(GLenum capability);
	void Disable(GLenum capability);
	bool IsEnabled(GLenum capability); // Not const because it might query GL and update cache

	// Blend Function
	void BlendFunc(GLenum sfactor, GLenum dfactor);
	void BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void BlendEquation(GLenum mode);
	void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);

	void GetBlendFunc(GLint& srcRGB, GLint& dstRGB, GLint& srcAlpha, GLint& dstAlpha);
	void GetBlendEquation(GLint& modeRGB, GLint& modeAlpha);

	// Viewport
	void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void GetViewport(GLint* viewport);

	// Framebuffer Binding
	void BindFramebuffer(GLenum target, GLuint framebuffer);
	GLuint GetFramebufferBinding(GLenum target);

	// Invalidate state (force next set/get to call GL)
	void Invalidate();

private:
	GLStateTracker();
	~GLStateTracker() = default;

	bool IsTracked(GLenum capability) const;

	// Capabilities: -1 = Unknown, 0 = Disabled, 1 = Enabled
	std::unordered_map<GLenum, int> capabilities_;

	// Blend State (-1 means unknown)
	GLint blend_src_rgb_ = -1;
	GLint blend_dst_rgb_ = -1;
	GLint blend_src_alpha_ = -1;
	GLint blend_dst_alpha_ = -1;
	GLint blend_eq_rgb_ = -1;
	GLint blend_eq_alpha_ = -1;

	// Viewport (-1 means unknown)
	std::array<GLint, 4> viewport_ = { -1, -1, -1, -1 };

	// Framebuffer Bindings
	struct CachedBinding {
		GLuint id = 0;
		bool valid = false;
	};
	CachedBinding read_framebuffer_;
	CachedBinding draw_framebuffer_;
};

#endif
