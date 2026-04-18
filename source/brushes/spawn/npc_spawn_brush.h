//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_NPC_SPAWN_BRUSH_H
#define RME_NPC_SPAWN_BRUSH_H

#include "brushes/brush.h"

class NpcSpawnBrush : public Brush {
public:
	NpcSpawnBrush();
	~NpcSpawnBrush() override;

	bool canDraw(BaseMap* map, const Position& position) const override;
	void draw(BaseMap* map, Tile* tile, void* parameter) override;
	void undraw(BaseMap* map, Tile* tile) override;

	int getLookID() const override;
	std::string getName() const override;
	bool canDrag() const override {
		return true;
	}
	bool canSmear() const override {
		return false;
	}
	bool oneSizeFitsAll() const override {
		return true;
	}
};

#endif
