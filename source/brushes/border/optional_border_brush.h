//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_OPTIONAL_BORDER_BRUSH_H_
#define RME_OPTIONAL_BORDER_BRUSH_H_

#include "brushes/brush.h"

class OptionalBorderBrush : public Brush {
public:
	OptionalBorderBrush();
	~OptionalBorderBrush() override = default;


	bool canDraw(BaseMap* map, const Position& position) const override;
	void draw(BaseMap* map, Tile* tile, const BrushContext& context) override;
	void undraw(BaseMap* map, Tile* tile) override;

	bool canDrag() const override {
		return true;
	}
	int getLookID() const override;
	std::string getName() const override;
};

#endif
