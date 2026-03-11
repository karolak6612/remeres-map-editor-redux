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

#ifndef RME_NVG_IMAGE_CACHE_H_
#define RME_NVG_IMAGE_CACHE_H_

#include <cstdint>
#include <unordered_map>

struct NVGcontext;
class GraphicManager;

// Cache for NanoVG image handles, keyed by item ID.
// Detects NVGcontext changes and invalidates the cache automatically.
// Can be shared by multiple NanoVG-based renderers.
class NVGImageCache {
public:
	explicit NVGImageCache(GraphicManager& graphics) : graphics_(graphics) {}
	~NVGImageCache();

	// Non-copyable, non-movable (owns NVG handles)
	NVGImageCache(const NVGImageCache&) = delete;
	NVGImageCache& operator=(const NVGImageCache&) = delete;

	// Get or create a NanoVG image handle for the given item ID.
	// Returns 0 on failure.
	int getSpriteImage(NVGcontext* vg, uint16_t itemId);

private:
	std::unordered_map<uint32_t, int> cache_;
	NVGcontext* last_context_ = nullptr;
	GraphicManager& graphics_;

	void invalidateAll();
};

#endif
