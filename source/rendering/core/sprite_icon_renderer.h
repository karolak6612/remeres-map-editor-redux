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

#ifndef RME_RENDERING_CORE_SPRITE_ICON_RENDERER_H_
#define RME_RENDERING_CORE_SPRITE_ICON_RENDERER_H_

#include <memory>
#include <unordered_map>
#include <wx/dc.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

#include "game/outfit.h"

enum SpriteSize : int;
class GameSprite;

class SpriteIconRenderer {
public:
	struct CachedDC {
		std::unique_ptr<wxMemoryDC> dc;
		std::unique_ptr<wxBitmap> bm;
	};

	struct RenderKey {
		SpriteSize size;
		uint32_t colorHash;
		uint32_t mountColorHash;
		int lookMount, lookAddon, lookMountHead, lookMountBody, lookMountLegs, lookMountFeet;

		bool operator==(const RenderKey& rk) const {
			return size == rk.size && colorHash == rk.colorHash && mountColorHash == rk.mountColorHash
				&& lookMount == rk.lookMount && lookAddon == rk.lookAddon
				&& lookMountHead == rk.lookMountHead && lookMountBody == rk.lookMountBody
				&& lookMountLegs == rk.lookMountLegs && lookMountFeet == rk.lookMountFeet;
		}
	};

	struct RenderKeyHash {
		size_t operator()(const RenderKey& k) const noexcept {
			size_t h = std::hash<uint64_t> {}((uint64_t(k.colorHash) << 32) | k.mountColorHash);
			h ^= std::hash<uint64_t> {}((uint64_t(k.lookMount) << 32) | k.lookAddon) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<uint64_t> {}((uint64_t(k.lookMountHead) << 32) | k.lookMountBody) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	void DrawTo(wxDC* dc, SpriteSize sz, GameSprite* sprite, int start_x, int start_y, int width, int height);
	void DrawTo(wxDC* dc, SpriteSize sz, GameSprite* sprite, const Outfit& outfit, int start_x, int start_y, int width, int height);
	void unloadDC();
	void eraseColoredDC(const RenderKey& key);

private:
	wxMemoryDC* getDC(SpriteSize size, GameSprite* sprite);
	wxMemoryDC* getDC(SpriteSize size, GameSprite* sprite, const Outfit& outfit);

	std::unique_ptr<wxMemoryDC> dc_[3]; // SPRITE_SIZE_COUNT
	std::unique_ptr<wxBitmap> bm_[3];
	std::unordered_map<RenderKey, std::unique_ptr<CachedDC>, RenderKeyHash> colored_dc_;
};

#endif
