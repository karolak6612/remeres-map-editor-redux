//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CREATURE_DRAWER_H_
#define RME_RENDERING_CREATURE_DRAWER_H_

#include "app/definitions.h"
#include "game/outfit.h"
#include "game/creature.h"
#include "rendering/core/tile_render_snapshot.h"

// Forward declarations

class SpriteDrawer;
class SpriteBatch;

#include <optional>
#include "map/position.h"

struct CreatureDrawOptions {
	DrawColor color;
	bool ingame = false;
	int animationPhase = 0;
	Position map_pos;
	std::optional<MapBounds> transient_selection_bounds;
};

class ISpriteResolver;

class CreatureDrawer {
public:
	CreatureDrawer();
	~CreatureDrawer();

	void BlitCreature(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options = {});
	void BlitCreature(
		SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, const CreatureRenderSnapshot& creature,
		const CreatureDrawOptions& options = {}
	);
	void BlitCreature(SpriteBatch& sprite_batch, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options = {});

	void SetSpriteResolver(ISpriteResolver* resolver) {
		sprite_resolver = resolver;
	}

private:
	ISpriteResolver* sprite_resolver = nullptr;
};

#endif
