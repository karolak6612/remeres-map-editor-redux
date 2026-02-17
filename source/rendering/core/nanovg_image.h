#ifndef RME_RENDERING_CORE_NANOVG_IMAGE_H_
#define RME_RENDERING_CORE_NANOVG_IMAGE_H_

#include <nanovg.h>

class NanoVGImage {
public:
	NanoVGImage(NVGcontext* ctx, int handle) :
		m_ctx(ctx), m_handle(handle) { }

	~NanoVGImage() {
		if (m_ctx && m_handle > 0) {
			nvgDeleteImage(m_ctx, m_handle);
		}
	}

	// Move-only
	NanoVGImage(const NanoVGImage&) = delete;
	NanoVGImage& operator=(const NanoVGImage&) = delete;

	NanoVGImage(NanoVGImage&& other) noexcept :
		m_ctx(other.m_ctx), m_handle(other.m_handle) {
		other.m_ctx = nullptr;
		other.m_handle = 0;
	}

	NanoVGImage& operator=(NanoVGImage&& other) noexcept {
		if (this != &other) {
			if (m_ctx && m_handle > 0) {
				nvgDeleteImage(m_ctx, m_handle);
			}
			m_ctx = other.m_ctx;
			m_handle = other.m_handle;
			other.m_ctx = nullptr;
			other.m_handle = 0;
		}
		return *this;
	}

	int getHandle() const {
		return m_handle;
	}
	bool isValid() const {
		return m_handle > 0;
	}

private:
	NVGcontext* m_ctx = nullptr;
	int m_handle = 0;
};

#endif
