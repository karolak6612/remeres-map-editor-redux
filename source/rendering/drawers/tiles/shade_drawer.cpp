#include "app/main.h"

// glut include removed

#include "rendering/drawers/tiles/shade_drawer.h"

ShadeDrawer::ShadeDrawer() {
}

ShadeDrawer::~ShadeDrawer() {
}

#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/view_state.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"

void ShadeDrawer::draw(const DrawContext& ctx, const FloorViewParams& floor_params) {
	if (ctx.view.start_z != ctx.view.end_z && ctx.options.show_shade) {
		glm::vec4 color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);
		float w = ctx.view.screensize_x * ctx.view.zoom;
		float h = ctx.view.screensize_y * ctx.view.zoom;

		if (g_gui.gfx.ensureAtlasManager()) {
			ctx.sprite_batch.drawRect(0.0f, 0.0f, w, h, color, *g_gui.gfx.getAtlasManager());
		}
	}
}
