#ifndef RME_RENDERING_CORE_NANOVG_IMAGE_H_
#define RME_RENDERING_CORE_NANOVG_IMAGE_H_

#include <nanovg.h>
#include <utility>

/**
 * RAII wrapper for NanoVG images.
 * Ensures that nvgDeleteImage is called when the object is destroyed.
 */
class NanoVGImage {
public:
	// Default constructor
	NanoVGImage() :
		vg(nullptr), image(0) { }

	// Constructor with context and image handle
	NanoVGImage(NVGcontext* vg, int image) :
		vg(vg), image(image) { }

	// Destructor
	~NanoVGImage() {
		reset();
	}

	// Move constructor
	NanoVGImage(NanoVGImage&& other) noexcept :
		vg(other.vg), image(other.image) {
		other.vg = nullptr;
		other.image = 0;
	}

	// Move assignment operator
	NanoVGImage& operator=(NanoVGImage&& other) noexcept {
		if (this != &other) {
			reset();
			vg = other.vg;
			image = other.image;
			other.vg = nullptr;
			other.image = 0;
		}
		return *this;
	}

	// Delete copy constructor and assignment
	NanoVGImage(const NanoVGImage&) = delete;
	NanoVGImage& operator=(const NanoVGImage&) = delete;

	// Reset the image (delete it)
	void reset() {
		if (vg && image > 0) {
			nvgDeleteImage(vg, image);
		}
		vg = nullptr;
		image = 0;
	}

	// Check if valid
	bool isValid() const {
		return vg != nullptr && image > 0;
	}

	// Get the image handle
	int get() const {
		return image;
	}

	// Implicit conversion to int handle
	operator int() const {
		return image;
	}

private:
	NVGcontext* vg;
	int image;
};

#endif // RME_RENDERING_CORE_NANOVG_IMAGE_H_
