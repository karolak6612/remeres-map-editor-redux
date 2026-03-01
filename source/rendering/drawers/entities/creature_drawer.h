//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CREATURE_DRAWER_H_
#define RME_RENDERING_CREATURE_DRAWER_H_

#include "app/definitions.h"
#include "game/outfit.h"
#include "game/creature.h"

// Forward declarations

class SpriteDrawer;
#include "rendering/core/draw_context.h"

#include <optional>
#include "map/position.h"

struct CreatureDrawOptions {
	DrawColor color;
	bool ingame = false;
	int animationPhase = 0;
	Position map_pos;
	std::optional<MapBounds> transient_selection_bounds;
};

class CreatureDrawer {
public:
	CreatureDrawer();
	~CreatureDrawer();

	void BlitCreature(const DrawContext& ctx, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options = {});
	void BlitCreature(const DrawContext& ctx, SpriteDrawer* sprite_drawer, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options = {});
};

#endif
