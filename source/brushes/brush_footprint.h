#ifndef RME_BRUSH_FOOTPRINT_H_
#define RME_BRUSH_FOOTPRINT_H_

#include "brushes/brush_enums.h"

struct BrushSizeState {
	BrushShape shape = BRUSHSHAPE_SQUARE;
	int size_x = 0;
	int size_y = 0;
	bool exact = false;
	bool aspect_locked = true;
};

struct BrushFootprint {
	BrushShape shape = BRUSHSHAPE_SQUARE;
	int size_x = 0;
	int size_y = 0;
	bool exact = false;
	int span_x = 1;
	int span_y = 1;
	int min_offset_x = 0;
	int max_offset_x = 0;
	int min_offset_y = 0;
	int max_offset_y = 0;

	[[nodiscard]] bool containsOffset(int dx, int dy) const;
	[[nodiscard]] bool isSingleTile() const {
		return span_x == 1 && span_y == 1;
	}
	[[nodiscard]] int maxReachX() const;
	[[nodiscard]] int maxReachY() const;
	[[nodiscard]] int legacySize() const;
};

[[nodiscard]] int NormalizeBrushAxisValue(int value, bool exact);
[[nodiscard]] BrushFootprint MakeBrushFootprint(const BrushSizeState& state);

#endif
