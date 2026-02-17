#ifndef RME_UTIL_NVG_UTILS_H_
#define RME_UTIL_NVG_UTILS_H_

#include "game/items.h"
#include "ui/gui.h"
#include <nanovg.h>
#include <vector>
#include <algorithm>
#include <memory>

namespace NvgUtils {

	// Generates RGBA pixel data for a given item ID.
	// Returns a texture ID created in the given NanoVG context.
	// Returns 0 if generation fails.
	inline int CreateItemTexture(NVGcontext* vg, uint16_t id) {
		const ItemType& it = g_items.getItemType(id);
		GameSprite* gs = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(it.clientID));
		if (!gs) {
			return 0;
		}

		int w, h;
		auto composite = gs->GetRGBAData(w, h);
		if (!composite) {
			return 0;
		}

		return nvgCreateImageRGBA(vg, w, h, 0, composite.get());
	}

} // namespace NvgUtils

#endif
