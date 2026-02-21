#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "ui/gui.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/sprite_sink.h"
#include "rendering/core/graphics.h"

#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "app/definitions.h"
#include <wx/gdicmn.h>

void GridDrawer::DrawGrid(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options) {
	if (!options.show_grid) {
		return;
	}

	glm::vec4 color(1.0f, 1.0f, 1.0f, 0.5f); // 128/255 approx 0.5

	if (g_gui.gfx.ensureAtlasManager()) {
		const AtlasManager& atlas = *g_gui.gfx.getAtlasManager();
		// Batch all horizontal lines
		for (int y = view.start_y; y < view.end_y; ++y) {
			float yPos = y * TILE_SIZE - view.view_scroll_y;
			float xStart = view.start_x * TILE_SIZE - view.view_scroll_x;
			float xEnd = view.end_x * TILE_SIZE - view.view_scroll_x;
			sprite_sink.drawRect(xStart, yPos, xEnd - xStart, 1.0f, color, atlas); // 1px height line
		}
		// Batch all vertical lines
		for (int x = view.start_x; x < view.end_x; ++x) {
			float xPos = x * TILE_SIZE - view.view_scroll_x;
			float yStart = view.start_y * TILE_SIZE - view.view_scroll_y;
			float yEnd = view.end_y * TILE_SIZE - view.view_scroll_y;
			sprite_sink.drawRect(xPos, yStart, 1.0f, yEnd - yStart, color, atlas); // 1px width line
		}
	}
}

void GridDrawer::DrawIngameBox(ISpriteSink& sprite_sink, const RenderView& view, const DrawingOptions& options) {
	if (!options.show_ingame_box) {
		return;
	}

	int center_x = view.start_x + int(view.screensize_x * view.zoom / 64);
	int center_y = view.start_y + int(view.screensize_y * view.zoom / 64);

	int offset_y = 2;
	int box_start_map_x = center_x;
	int box_start_map_y = center_y + offset_y;
	int box_end_map_x = center_x + ClientMapWidth;
	int box_end_map_y = center_y + ClientMapHeight + offset_y;

	int box_start_x = box_start_map_x * TILE_SIZE - view.view_scroll_x;
	int box_start_y = box_start_map_y * TILE_SIZE - view.view_scroll_y;
	int box_end_x = box_end_map_x * TILE_SIZE - view.view_scroll_x;
	int box_end_y = box_end_map_y * TILE_SIZE - view.view_scroll_y;

	static wxColor side_color(0, 0, 0, 200);

	// BatchRenderer doesn't support disabling GL_TEXTURE_2D state globally,
	// but DrawQuad uses white texture if no texture ID is provided or if specific non-textured method used.
	// DrawQuad uses whiteTextureID by default.

	// left side
	if (box_start_map_x >= view.start_x) {
		drawFilledRect(sprite_sink, 0, 0, box_start_x, view.screensize_y * view.zoom, side_color);
	}

	// right side
	if (box_end_map_x < view.end_x) {
		drawFilledRect(sprite_sink, box_end_x, 0, view.screensize_x * view.zoom, view.screensize_y * view.zoom, side_color);
	}

	// top side
	if (box_start_map_y >= view.start_y) {
		drawFilledRect(sprite_sink, box_start_x, 0, box_end_x - box_start_x, box_start_y, side_color);
	}

	// bottom side
	if (box_end_map_y < view.end_y) {
		drawFilledRect(sprite_sink, box_start_x, box_end_y, box_end_x - box_start_x, view.screensize_y * view.zoom, side_color);
	}

	// hidden tiles
	drawRect(sprite_sink, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxRED);

	// visible tiles
	box_start_x += TILE_SIZE;
	box_start_y += TILE_SIZE;
	box_end_x -= 1 * TILE_SIZE;
	box_end_y -= 1 * TILE_SIZE;
	drawRect(sprite_sink, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);

	// player position
	box_start_x += (ClientMapWidth - 3) / 2 * TILE_SIZE;
	box_start_y += (ClientMapHeight - 3) / 2 * TILE_SIZE;
	box_end_x = box_start_x + TILE_SIZE;
	box_end_y = box_start_y + TILE_SIZE;
	drawRect(sprite_sink, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);
}

void GridDrawer::DrawNodeLoadingPlaceholder(ISpriteSink& sprite_sink, int nd_map_x, int nd_map_y, const RenderView& view) {
	int cy = (nd_map_y)*TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
	int cx = (nd_map_x)*TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();

	glm::vec4 color(1.0f, 0.0f, 1.0f, 0.5f); // 255, 0, 255, 128

	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_sink.drawRect((float)cx, (float)cy, (float)TILE_SIZE * 4, (float)TILE_SIZE * 4, color, *g_gui.gfx.getAtlasManager());
	}
}

void GridDrawer::drawRect(ISpriteSink& sprite_sink, int x, int y, int w, int h, const wxColor& color, int width) {
	// glLineWidth(width); // Width ignored for now, BatchRenderer lines are 1px
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.gfx.ensureAtlasManager()) {
		const AtlasManager& am = *g_gui.gfx.getAtlasManager();
		// Top
		sprite_sink.drawRect((float)x, (float)y, (float)w, 1.0f, c, am);
		// Bottom
		sprite_sink.drawRect((float)x, (float)y + h - 1.0f, (float)w, 1.0f, c, am);
		// Left
		sprite_sink.drawRect((float)x, (float)y + 1.0f, 1.0f, (float)h - 2.0f, c, am);
		// Right
		sprite_sink.drawRect((float)x + w - 1.0f, (float)y + 1.0f, 1.0f, (float)h - 2.0f, c, am);
	}
}

void GridDrawer::drawFilledRect(ISpriteSink& sprite_sink, int x, int y, int w, int h, const wxColor& color) {
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_sink.drawRect((float)x, (float)y, (float)w, (float)h, c, *g_gui.gfx.getAtlasManager());
	}
}

