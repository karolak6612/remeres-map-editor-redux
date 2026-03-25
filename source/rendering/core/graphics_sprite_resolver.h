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

#ifndef RME_RENDERING_CORE_GRAPHICS_SPRITE_RESOLVER_H_
#define RME_RENDERING_CORE_GRAPHICS_SPRITE_RESOLVER_H_

#include "rendering/core/sprite_resolver.h"
#include "rendering/core/graphics.h"
#include <cassert>

// Concrete ISpriteResolver implementation that delegates to GraphicManager.
// Wraps the global GraphicManager to provide sprite lookup through the
// ISpriteResolver interface, enabling testability and decoupling from the singleton.
class GraphicsSpriteResolver : public ISpriteResolver {
public:
	explicit GraphicsSpriteResolver(GraphicManager& gfx) : gfx_(gfx) {}

	GameSprite* getSprite(int client_id) override {
		Sprite* s = gfx_.getSprite(client_id);
		assert(s == nullptr || dynamic_cast<GameSprite*>(s) != nullptr);
		return static_cast<GameSprite*>(s);
	}

	GameSprite* getCreatureSprite(int look_type) override {
		return gfx_.getCreatureSprite(look_type);
	}

	const SpriteMetadata* getSpriteMetadata(int client_id) const override {
		return gfx_.getSpriteMetadata(client_id);
	}

	SpriteAnimationState* getSpriteAnimation(int client_id) override {
		return gfx_.getSpriteAnimation(client_id);
	}

	bool isSpriteSimpleAndLoaded(int client_id) const override {
		return gfx_.isSpriteSimpleAndLoaded(client_id);
	}

	const AtlasRegion* getItemAtlasRegion(
		int client_id, int x, int y, int layer, int subtype, int pattern_x, int pattern_y, int pattern_z, int frame
	) override {
		return gfx_.getItemAtlasRegion(client_id, x, y, layer, subtype, pattern_x, pattern_y, pattern_z, frame);
	}

	const AtlasRegion* getCreatureAtlasRegion(
		int client_id, int x, int y, int dir, int addon, int pattern_z, const Outfit& outfit, int frame
	) override {
		return gfx_.getCreatureAtlasRegion(client_id, x, y, dir, addon, pattern_z, outfit, frame);
	}

private:
	GraphicManager& gfx_;
};

#endif
