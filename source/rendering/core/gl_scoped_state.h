#ifndef RME_RENDERING_CORE_GL_SCOPED_STATE_H_
#define RME_RENDERING_CORE_GL_SCOPED_STATE_H_

#include "rendering/core/gl_state_tracker.h"
#include <glad/glad.h>
#include <cassert>
#include <utility>

/**
 * @brief RAII wrapper for glEnable/glDisable using GLStateTracker
 *
 * Intended for short-lived scope-based state changes, not for persistent state modifications.
 */
class ScopedGLCapability {
public:
	[[nodiscard]] explicit ScopedGLCapability(GLenum capability, bool enable = true) :
		capability_(capability) {
		was_enabled_ = GLStateTracker::Instance().IsEnabled(capability);
		if (enable) {
			if (!was_enabled_) {
				GLStateTracker::Instance().Enable(capability);
			}
		} else {
			if (was_enabled_) {
				GLStateTracker::Instance().Disable(capability);
			}
		}
	}

	~ScopedGLCapability() {
		restore();
	}

	ScopedGLCapability(const ScopedGLCapability&) = delete;
	ScopedGLCapability& operator=(const ScopedGLCapability&) = delete;

	ScopedGLCapability(ScopedGLCapability&& other) noexcept :
		capability_(other.capability_),
		was_enabled_(other.was_enabled_),
		active_(std::exchange(other.active_, false)) {
	}

	ScopedGLCapability& operator=(ScopedGLCapability&& other) noexcept {
		if (this != &other) {
			restore();
			capability_ = other.capability_;
			was_enabled_ = other.was_enabled_;
			active_ = std::exchange(other.active_, false);
		}
		return *this;
	}

private:
	void restore() const {
		if (active_) {
			if (was_enabled_) {
				GLStateTracker::Instance().Enable(capability_);
			} else {
				GLStateTracker::Instance().Disable(capability_);
			}
		}
	}

private:
	GLenum capability_;
	bool was_enabled_;
	bool active_ = true;
};

/**
 * @brief RAII wrapper for glBlendFunc and glBlendEquation using GLStateTracker
 *
 * Intended for short-lived scope-based state changes, not for persistent state modifications.
 */
class ScopedGLBlend {
public:
	[[nodiscard]] ScopedGLBlend(GLenum sfactor, GLenum dfactor) {
		save_state();
		GLStateTracker::Instance().BlendFunc(sfactor, dfactor);
	}

	[[nodiscard]] ScopedGLBlend(GLenum sfactor, GLenum dfactor, GLenum equation) {
		save_state();
		GLStateTracker::Instance().BlendFunc(sfactor, dfactor);
		GLStateTracker::Instance().BlendEquation(equation);
	}

	~ScopedGLBlend() {
		restore();
	}

	ScopedGLBlend(const ScopedGLBlend&) = delete;
	ScopedGLBlend& operator=(const ScopedGLBlend&) = delete;

	ScopedGLBlend(ScopedGLBlend&& other) noexcept :
		prev_src_rgb_(other.prev_src_rgb_),
		prev_dst_rgb_(other.prev_dst_rgb_),
		prev_src_alpha_(other.prev_src_alpha_),
		prev_dst_alpha_(other.prev_dst_alpha_),
		prev_eq_rgb_(other.prev_eq_rgb_),
		prev_eq_alpha_(other.prev_eq_alpha_),
		active_(std::exchange(other.active_, false)) {
	}

	ScopedGLBlend& operator=(ScopedGLBlend&& other) noexcept {
		if (this != &other) {
			restore();
			prev_src_rgb_ = other.prev_src_rgb_;
			prev_dst_rgb_ = other.prev_dst_rgb_;
			prev_src_alpha_ = other.prev_src_alpha_;
			prev_dst_alpha_ = other.prev_dst_alpha_;
			prev_eq_rgb_ = other.prev_eq_rgb_;
			prev_eq_alpha_ = other.prev_eq_alpha_;
			active_ = std::exchange(other.active_, false);
		}
		return *this;
	}

private:
	void restore() const {
		if (active_) {
			GLStateTracker::Instance().BlendFuncSeparate(prev_src_rgb_, prev_dst_rgb_, prev_src_alpha_, prev_dst_alpha_);
			GLStateTracker::Instance().BlendEquationSeparate(prev_eq_rgb_, prev_eq_alpha_);
		}
	}

private:
	void save_state() {
		GLStateTracker::Instance().GetBlendFunc(prev_src_rgb_, prev_dst_rgb_, prev_src_alpha_, prev_dst_alpha_);
		GLStateTracker::Instance().GetBlendEquation(prev_eq_rgb_, prev_eq_alpha_);
	}

	GLint prev_src_rgb_ = GL_ONE, prev_dst_rgb_ = GL_ZERO;
	GLint prev_src_alpha_ = GL_ONE, prev_dst_alpha_ = GL_ZERO;
	GLint prev_eq_rgb_ = GL_FUNC_ADD, prev_eq_alpha_ = GL_FUNC_ADD;
	bool active_ = true;
};

/**
 * @brief RAII wrapper for glFramebuffer using GLStateTracker
 *
 * Saves current READ and DRAW framebuffer bindings on construction and restores them on destruction.
 */
class ScopedGLFramebuffer {
public:
	[[nodiscard]] explicit ScopedGLFramebuffer(GLenum target, GLuint framebuffer) :
		target_(target) {
		assert(target_ == GL_FRAMEBUFFER || target_ == GL_READ_FRAMEBUFFER || target_ == GL_DRAW_FRAMEBUFFER);

		if (target_ == GL_FRAMEBUFFER || target_ == GL_READ_FRAMEBUFFER) {
			prev_read_ = GLStateTracker::Instance().GetFramebufferBinding(GL_READ_FRAMEBUFFER);
		}
		if (target_ == GL_FRAMEBUFFER || target_ == GL_DRAW_FRAMEBUFFER) {
			prev_draw_ = GLStateTracker::Instance().GetFramebufferBinding(GL_DRAW_FRAMEBUFFER);
		}

		GLStateTracker::Instance().BindFramebuffer(target_, framebuffer);
	}

	~ScopedGLFramebuffer() {
		restore();
	}

	ScopedGLFramebuffer(const ScopedGLFramebuffer&) = delete;
	ScopedGLFramebuffer& operator=(const ScopedGLFramebuffer&) = delete;

	ScopedGLFramebuffer(ScopedGLFramebuffer&& other) noexcept :
		target_(other.target_),
		prev_read_(other.prev_read_),
		prev_draw_(other.prev_draw_),
		active_(std::exchange(other.active_, false)) {
	}

	ScopedGLFramebuffer& operator=(ScopedGLFramebuffer&& other) noexcept {
		if (this != &other) {
			restore();
			target_ = other.target_;
			prev_read_ = other.prev_read_;
			prev_draw_ = other.prev_draw_;
			active_ = std::exchange(other.active_, false);
		}
		return *this;
	}

private:
	void restore() const {
		if (active_) {
			if (target_ == GL_FRAMEBUFFER) {
				if (prev_read_ == prev_draw_) {
					GLStateTracker::Instance().BindFramebuffer(GL_FRAMEBUFFER, prev_read_);
				} else {
					GLStateTracker::Instance().BindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_);
					GLStateTracker::Instance().BindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_);
				}
			} else if (target_ == GL_READ_FRAMEBUFFER) {
				GLStateTracker::Instance().BindFramebuffer(GL_READ_FRAMEBUFFER, prev_read_);
			} else { // GL_DRAW_FRAMEBUFFER
				GLStateTracker::Instance().BindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_);
			}
		}
	}

private:
	GLenum target_;
	GLuint prev_read_ = 0;
	GLuint prev_draw_ = 0;
	bool active_ = true;
};

/**
 * @brief RAII wrapper for glViewport using GLStateTracker
 *
 * Saves current viewport on construction and restores it on destruction.
 */
class ScopedGLViewport {
public:
	[[nodiscard]] ScopedGLViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
		GLStateTracker::Instance().GetViewport(prev_viewport_);
		GLStateTracker::Instance().Viewport(x, y, width, height);
	}

	~ScopedGLViewport() {
		restore();
	}

	ScopedGLViewport(const ScopedGLViewport&) = delete;
	ScopedGLViewport& operator=(const ScopedGLViewport&) = delete;

	ScopedGLViewport(ScopedGLViewport&& other) noexcept :
		active_(std::exchange(other.active_, false)) {
		for (int i = 0; i < 4; ++i) {
			prev_viewport_[i] = other.prev_viewport_[i];
		}
	}

	ScopedGLViewport& operator=(ScopedGLViewport&& other) noexcept {
		if (this != &other) {
			restore();
			for (int i = 0; i < 4; ++i) {
				prev_viewport_[i] = other.prev_viewport_[i];
			}
			active_ = std::exchange(other.active_, false);
		}
		return *this;
	}

private:
	void restore() const {
		if (active_) {
			GLStateTracker::Instance().Viewport(prev_viewport_[0], prev_viewport_[1], prev_viewport_[2], prev_viewport_[3]);
		}
	}

private:
	GLint prev_viewport_[4];
	bool active_ = true;
};

#endif
