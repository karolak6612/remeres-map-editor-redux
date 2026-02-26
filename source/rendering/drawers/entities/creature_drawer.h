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
class SpriteBatch;
class RenderList;

struct CreatureDrawOptions {
	DrawColor color;
	bool ingame = false;
	int animationPhase = 0;
};

class CreatureDrawer {
public:
	CreatureDrawer(SpriteDrawer* sprite_drawer);
	~CreatureDrawer();

	void BlitCreature(SpriteBatch& sprite_batch, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options = {});
	void BlitCreature(SpriteBatch& sprite_batch, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options = {});

	void BlitCreature(RenderList& list, int screenx, int screeny, const Creature* c, const CreatureDrawOptions& options = {});
	void BlitCreature(RenderList& list, int screenx, int screeny, const Outfit& outfit, Direction dir, const CreatureDrawOptions& options = {});

private:
	SpriteDrawer* sprite_drawer;
};

#endif
