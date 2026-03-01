#ifndef RME_UTIL_NVG_UTILS_H_
#define RME_UTIL_NVG_UTILS_H_

#include <wx/colour.h>
#include <nanovg.h>
#include <memory>
#include <cstdint>

struct NVGcontext;
struct NVGDeleter {
	void operator()(NVGcontext* nvg) const;
};

class GameSprite;

namespace NvgUtils {

	// Generates RGBA pixel data for a given item ID.
	// Returns a texture ID created in the given NanoVG context.
	// Returns 0 if generation fails.
	inline NVGcolor ToNvColor(const wxColour& c) {
		return nvgRGBA(c.Red(), c.Green(), c.Blue(), c.Alpha());
	}

	// Creates a composite RGBA buffer from a GameSprite.
	// Returns a unique_ptr to the buffer, or nullptr on failure.
	std::unique_ptr<uint8_t[]> CreateCompositeRGBA(GameSprite& gs, int& outW, int& outH);

	// Generates RGBA pixel data for a given item ID.
	// Returns a texture ID created in the given NanoVG context.
	// Returns 0 if generation fails.
	int CreateItemTexture(NVGcontext* vg, uint16_t id);

} // namespace NvgUtils

#endif
