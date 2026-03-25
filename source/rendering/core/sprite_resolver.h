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

#ifndef RME_RENDERING_CORE_SPRITE_RESOLVER_H_
#define RME_RENDERING_CORE_SPRITE_RESOLVER_H_

class GameSprite;
struct AtlasRegion;
struct Outfit;
struct SpriteAnimationState;
struct SpriteMetadata;

// Abstract interface for resolving sprites by ID.
// Decouples rendering from the global GraphicManager singleton.
class ISpriteResolver {
public:
	virtual ~ISpriteResolver() = default;
	virtual GameSprite* getSprite(int client_id) = 0;
	virtual GameSprite* getCreatureSprite(int look_type) = 0;
	virtual const SpriteMetadata* getSpriteMetadata(int client_id) const = 0;
	virtual SpriteAnimationState* getSpriteAnimation(int client_id) = 0;
	virtual bool isSpriteSimpleAndLoaded(int client_id) const = 0;
	virtual const AtlasRegion* getItemAtlasRegion(
		int client_id, int x, int y, int layer, int subtype, int pattern_x, int pattern_y, int pattern_z, int frame
	) = 0;
	virtual const AtlasRegion* getCreatureAtlasRegion(
		int client_id, int x, int y, int dir, int addon, int pattern_z, const Outfit& outfit, int frame
	) = 0;
};

#endif
