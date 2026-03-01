#ifndef RME_SHADE_DRAWER_H_
#define RME_SHADE_DRAWER_H_

#include "rendering/core/draw_context.h"
#include "rendering/core/floor_view_params.h"

class ShadeDrawer {
public:
	ShadeDrawer();
	~ShadeDrawer();

	void draw(const DrawContext& ctx, const FloorViewParams& floor_params);
};

#endif
