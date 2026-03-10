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

#include "app/main.h"
#include "rendering/core/sprite_icon_renderer.h"
#include "rendering/core/game_sprite.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"

void SpriteIconRenderer::DrawTo(wxDC* dc, SpriteSize sz, GameSprite* sprite, int start_x, int start_y, int width, int height) {
	const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
	int src_width = sprite_dim;
	int src_height = sprite_dim;

	if (width == -1) {
		width = src_width;
	}
	if (height == -1) {
		height = src_height;
	}
	wxDC* sdc = getDC(sz, sprite);
	if (sdc) {
		dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
	} else {
		const wxBrush& b = dc->GetBrush();
		dc->SetBrush(*wxRED_BRUSH);
		dc->DrawRectangle(start_x, start_y, width, height);
		dc->SetBrush(b);
	}
}

void SpriteIconRenderer::DrawTo(wxDC* dc, SpriteSize sz, GameSprite* sprite, const Outfit& outfit, int start_x, int start_y, int width, int height) {
	const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
	int src_width = sprite_dim;
	int src_height = sprite_dim;

	if (width == -1) {
		width = src_width;
	}
	if (height == -1) {
		height = src_height;
	}
	wxDC* sdc = getDC(sz, sprite, outfit);
	if (sdc) {
		dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
	} else {
		const wxBrush& b = dc->GetBrush();
		dc->SetBrush(*wxRED_BRUSH);
		dc->DrawRectangle(start_x, start_y, width, height);
		dc->SetBrush(b);
	}
}

void SpriteIconRenderer::unloadDC() {
	dc_[SPRITE_SIZE_16x16].reset();
	dc_[SPRITE_SIZE_32x32].reset();
	dc_[SPRITE_SIZE_64x64].reset();
	bm_[SPRITE_SIZE_16x16].reset();
	bm_[SPRITE_SIZE_32x32].reset();
	bm_[SPRITE_SIZE_64x64].reset();
	colored_dc_.clear();
}

void SpriteIconRenderer::eraseColoredDC(const RenderKey& key) {
	colored_dc_.erase(key);
}

wxMemoryDC* SpriteIconRenderer::getDC(SpriteSize size, GameSprite* sprite) {
	ASSERT(size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_32x32 || size == SPRITE_SIZE_64x64);

	if (!dc_[size]) {
		wxBitmap bmp = SpriteIconGenerator::Generate(sprite, size);
		if (bmp.IsOk()) {
			bm_[size] = std::make_unique<wxBitmap>(bmp);
			dc_[size] = std::make_unique<wxMemoryDC>(*bm_[size]);
		}
		g_gui.gfx.addSpriteToCleanup(sprite);
	}
	return dc_[size].get();
}

wxMemoryDC* SpriteIconRenderer::getDC(SpriteSize size, GameSprite* sprite, const Outfit& outfit) {
	ASSERT(size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_32x32 || size == SPRITE_SIZE_64x64);

	RenderKey key;
	key.size = size;
	key.colorHash = outfit.getColorHash();
	key.mountColorHash = outfit.getMountColorHash();
	key.lookMount = outfit.lookMount;
	key.lookAddon = outfit.lookAddon;
	key.lookMountHead = outfit.lookMountHead;
	key.lookMountBody = outfit.lookMountBody;
	key.lookMountLegs = outfit.lookMountLegs;
	key.lookMountFeet = outfit.lookMountFeet;

	auto it = colored_dc_.find(key);
	if (it == colored_dc_.end()) {
		wxBitmap bmp = SpriteIconGenerator::Generate(sprite, size, outfit);
		if (bmp.IsOk()) {
			auto cache = std::make_unique<CachedDC>();
			cache->bm = std::make_unique<wxBitmap>(bmp);
			cache->dc = std::make_unique<wxMemoryDC>(*cache->bm);

			auto res = colored_dc_.insert(std::make_pair(key, std::move(cache)));
			g_gui.gfx.addSpriteToCleanup(sprite);
			return res.first->second->dc.get();
		}
		return nullptr;
	}
	return it->second->dc.get();
}
