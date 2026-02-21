#include "app/main.h"

// glut include removed

#include "rendering/drawers/tiles/shade_drawer.h"

ShadeDrawer::ShadeDrawer() {
}

ShadeDrawer::~ShadeDrawer() {
}

#include "rendering/core/sprite_sink.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"

void ShadeDrawer::draw(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options) {
	if (view.start_z != view.end_z && options.show_shade) {
		glm::vec4 color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);
		float w = view.screensize_x * view.zoom;
		float h = view.screensize_y * view.zoom;

		if (g_gui.gfx.ensureAtlasManager()) {
			sprite_sink.drawRect(0.0f, 0.0f, w, h, color, *g_gui.gfx.getAtlasManager());
		}
	}
}
