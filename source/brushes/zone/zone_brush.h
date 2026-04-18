//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ZONE_BRUSH_H
#define RME_ZONE_BRUSH_H

#include "brushes/brush.h"

class ZoneBrush : public Brush {
public:
	ZoneBrush();
	~ZoneBrush() override;

	bool canDraw(BaseMap* map, const Position& position) const override;
	void draw(BaseMap* map, Tile* tile, void* parameter) override;
	void undraw(BaseMap* map, Tile* tile) override;

	int getLookID() const override;
	std::string getName() const override;
	bool canDrag() const override {
		return true;
	}
	bool canSmear() const override {
		return true;
	}
};

#endif
