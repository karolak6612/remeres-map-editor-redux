#include "rendering/core/gl_state_tracker.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

GLStateTracker& GLStateTracker::Instance() {
	static GLStateTracker instance;
	return instance;
}

GLStateTracker::GLStateTracker() {
	Invalidate();
}

void GLStateTracker::Invalidate() {
	// Capabilities: -1 means unknown
	capabilities_.clear();

	// Blend State (-1 means unknown)
	blend_src_rgb_ = -1;
	blend_dst_rgb_ = -1;
	blend_src_alpha_ = -1;
	blend_dst_alpha_ = -1;
	blend_eq_rgb_ = -1;
	blend_eq_alpha_ = -1;

	// Viewport (-1 means unknown)
	viewport_ = { -1, -1, -1, -1 };

	read_framebuffer_.valid = false;
	draw_framebuffer_.valid = false;
}

void GLStateTracker::Enable(GLenum capability) {
	if (IsTracked(capability)) {
		int current = capabilities_.count(capability) ? capabilities_[capability] : -1;
		if (current != 1) {
			glEnable(capability);
			capabilities_[capability] = 1;
		}
	} else {
		glEnable(capability);
	}
}

void GLStateTracker::Disable(GLenum capability) {
	if (IsTracked(capability)) {
		int current = capabilities_.count(capability) ? capabilities_[capability] : -1;
		if (current != 0) {
			glDisable(capability);
			capabilities_[capability] = 0;
		}
	} else {
		glDisable(capability);
	}
}

bool GLStateTracker::IsEnabled(GLenum capability) {
	if (IsTracked(capability)) {
		int current = capabilities_.count(capability) ? capabilities_[capability] : -1;
		if (current != -1) {
			return current == 1;
		}
	}
	// Fallback to query if dirty or untracked
	bool enabled = glIsEnabled(capability);
	if (IsTracked(capability)) {
		capabilities_[capability] = enabled ? 1 : 0;
	}
	return enabled;
}

void GLStateTracker::BlendFunc(GLenum sfactor, GLenum dfactor) {
	BlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void GLStateTracker::BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
	if (blend_src_rgb_ != srcRGB || blend_dst_rgb_ != dstRGB || blend_src_alpha_ != srcAlpha || blend_dst_alpha_ != dstAlpha) {
		glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
		blend_src_rgb_ = srcRGB;
		blend_dst_rgb_ = dstRGB;
		blend_src_alpha_ = srcAlpha;
		blend_dst_alpha_ = dstAlpha;
	}
}

void GLStateTracker::BlendEquation(GLenum mode) {
	BlendEquationSeparate(mode, mode);
}

void GLStateTracker::BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
	if (blend_eq_rgb_ != modeRGB || blend_eq_alpha_ != modeAlpha) {
		glBlendEquationSeparate(modeRGB, modeAlpha);
		blend_eq_rgb_ = modeRGB;
		blend_eq_alpha_ = modeAlpha;
	}
}

void GLStateTracker::GetBlendFunc(GLint& srcRGB, GLint& dstRGB, GLint& srcAlpha, GLint& dstAlpha) {
	if (blend_src_rgb_ != -1) {
		srcRGB = blend_src_rgb_;
		dstRGB = blend_dst_rgb_;
		srcAlpha = blend_src_alpha_;
		dstAlpha = blend_dst_alpha_;
	} else {
		glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
		glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);

		blend_src_rgb_ = srcRGB;
		blend_dst_rgb_ = dstRGB;
		blend_src_alpha_ = srcAlpha;
		blend_dst_alpha_ = dstAlpha;
	}
}

void GLStateTracker::GetBlendEquation(GLint& modeRGB, GLint& modeAlpha) {
	if (blend_eq_rgb_ != -1) {
		modeRGB = blend_eq_rgb_;
		modeAlpha = blend_eq_alpha_;
	} else {
		glGetIntegerv(GL_BLEND_EQUATION_RGB, &modeRGB);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &modeAlpha);

		blend_eq_rgb_ = modeRGB;
		blend_eq_alpha_ = modeAlpha;
	}
}

void GLStateTracker::Viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	if (viewport_[0] != x || viewport_[1] != y || viewport_[2] != width || viewport_[3] != height) {
		glViewport(x, y, width, height);
		viewport_[0] = x;
		viewport_[1] = y;
		viewport_[2] = width;
		viewport_[3] = height;
	}
}

void GLStateTracker::GetViewport(GLint* viewport) {
	if (viewport_[0] != -1) {
		viewport[0] = viewport_[0];
		viewport[1] = viewport_[1];
		viewport[2] = viewport_[2];
		viewport[3] = viewport_[3];
	} else {
		glGetIntegerv(GL_VIEWPORT, viewport);
		viewport_[0] = viewport[0];
		viewport_[1] = viewport[1];
		viewport_[2] = viewport[2];
		viewport_[3] = viewport[3];
	}
}

void GLStateTracker::BindFramebuffer(GLenum target, GLuint framebuffer) {
	bool updated = false;

	if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
		if (!read_framebuffer_.valid || read_framebuffer_.id != framebuffer) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
			read_framebuffer_.id = framebuffer;
			read_framebuffer_.valid = true;
			updated = true;
		}
	}
	if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
		if (!draw_framebuffer_.valid || draw_framebuffer_.id != framebuffer) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
			draw_framebuffer_.id = framebuffer;
			draw_framebuffer_.valid = true;
			updated = true;
		}
	}
}

GLuint GLStateTracker::GetFramebufferBinding(GLenum target) {
	if (target == GL_READ_FRAMEBUFFER) {
		if (read_framebuffer_.valid) return read_framebuffer_.id;
	}
	if (target == GL_DRAW_FRAMEBUFFER) {
		if (draw_framebuffer_.valid) return draw_framebuffer_.id;
	}
	// GL_FRAMEBUFFER returns DRAW binding usually
	if (target == GL_FRAMEBUFFER) {
		if (draw_framebuffer_.valid) return draw_framebuffer_.id;
	}

	// Fallback
	GLint binding = 0;
	if (target == GL_READ_FRAMEBUFFER) {
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &binding);
		read_framebuffer_.id = static_cast<GLuint>(binding);
		read_framebuffer_.valid = true;
	} else if (target == GL_DRAW_FRAMEBUFFER) {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &binding);
		draw_framebuffer_.id = static_cast<GLuint>(binding);
		draw_framebuffer_.valid = true;
	} else {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &binding); // Default for GL_FRAMEBUFFER
		draw_framebuffer_.id = static_cast<GLuint>(binding);
		draw_framebuffer_.valid = true;
	}
	return static_cast<GLuint>(binding);
}

bool GLStateTracker::IsTracked(GLenum capability) const {
	// Only track specific capabilities for now
	return capability == GL_BLEND || capability == GL_DEPTH_TEST || capability == GL_CULL_FACE || capability == GL_SCISSOR_TEST || capability == GL_STENCIL_TEST;
}
