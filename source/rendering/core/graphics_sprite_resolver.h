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

// Concrete ISpriteResolver implementation that delegates to GraphicManager.
// Wraps the global GraphicManager to provide sprite lookup through the
// ISpriteResolver interface, enabling testability and decoupling from the singleton.
class GraphicsSpriteResolver : public ISpriteResolver {
public:
	explicit GraphicsSpriteResolver(GraphicManager& gfx) : gfx_(gfx) {}

	GameSprite* getSprite(int client_id) override {
		return dynamic_cast<GameSprite*>(gfx_.getSprite(client_id));
	}

	GameSprite* getCreatureSprite(int look_type) override {
		return gfx_.getCreatureSprite(look_type);
	}

private:
	GraphicManager& gfx_;
};

#endif
