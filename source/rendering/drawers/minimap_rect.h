#ifndef RME_RENDERING_DRAWERS_MINIMAP_RECT_H_
#define RME_RENDERING_DRAWERS_MINIMAP_RECT_H_

#include <algorithm>

struct MinimapDirtyRect {
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
};

[[nodiscard]] inline bool IsValidMinimapRect(const MinimapDirtyRect& rect) {
	return rect.width > 0 && rect.height > 0;
}

[[nodiscard]] inline MinimapDirtyRect UnionMinimapRects(const MinimapDirtyRect& lhs, const MinimapDirtyRect& rhs) {
	if (!IsValidMinimapRect(lhs)) {
		return rhs;
	}
	if (!IsValidMinimapRect(rhs)) {
		return lhs;
	}

	const int min_x = std::min(lhs.x, rhs.x);
	const int min_y = std::min(lhs.y, rhs.y);
	const int max_x = std::max(lhs.x + lhs.width, rhs.x + rhs.width);
	const int max_y = std::max(lhs.y + lhs.height, rhs.y + rhs.height);
	return {
		.x = min_x,
		.y = min_y,
		.width = std::max(0, max_x - min_x),
		.height = std::max(0, max_y - min_y),
	};
}

#endif
