#include "rendering/drawers/overlays/grid_drawer.h"
#include "ui/gui.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/drawing_options.h"
#include "app/definitions.h"
#include <wx/gdicmn.h>

void GridDrawer::DrawGrid(const DrawContext& ctx, const ViewBounds& bounds) {
	if (!ctx.options.show_grid) {
		return;
	}

	glm::vec4 color(0.0f, 0.0f, 0.0f, 0.50f); // Subtle dark overlay

	// Use zoom as line thickness so lines are always ~1 screen pixel
	const float line_thickness = ctx.view.zoom;

	if (g_gui.atlas.ensureAtlasManager()) {
		const AtlasManager& atlas = *g_gui.atlas.getAtlasManager();

		// Hoisted invariants for horizontal lines
		const float h_xStart = bounds.start_x * TILE_SIZE - ctx.view.view_scroll_x;
		const float h_xEnd = bounds.end_x * TILE_SIZE - ctx.view.view_scroll_x;
		const float h_width = h_xEnd - h_xStart;

		for (int y = bounds.start_y; y <= bounds.end_y; ++y) {
			float yPos = y * TILE_SIZE - ctx.view.view_scroll_y;
			ctx.sprite_batch.drawRect(h_xStart, yPos, h_width, line_thickness, color, atlas);
		}

		// Hoisted invariants for vertical lines
		const float v_yStart = bounds.start_y * TILE_SIZE - ctx.view.view_scroll_y;
		const float v_yEnd = bounds.end_y * TILE_SIZE - ctx.view.view_scroll_y;
		const float v_height = v_yEnd - v_yStart;

		for (int x = bounds.start_x; x <= bounds.end_x; ++x) {
			float xPos = x * TILE_SIZE - ctx.view.view_scroll_x;
			ctx.sprite_batch.drawRect(xPos, v_yStart, line_thickness, v_height, color, atlas);
		}
	}
}

void GridDrawer::DrawIngameBox(const DrawContext& ctx, const ViewBounds& bounds) {
	if (!ctx.options.show_ingame_box) {
		return;
	}

	int center_x = bounds.start_x + int(ctx.view.screensize_x * ctx.view.zoom / 64);
	int center_y = bounds.start_y + int(ctx.view.screensize_y * ctx.view.zoom / 64);

	int offset_y = 2;
	int box_start_map_x = center_x;
	int box_start_map_y = center_y + offset_y;
	int box_end_map_x = center_x + ClientMapWidth;
	int box_end_map_y = center_y + ClientMapHeight + offset_y;

	int box_start_x = box_start_map_x * TILE_SIZE - ctx.view.view_scroll_x;
	int box_start_y = box_start_map_y * TILE_SIZE - ctx.view.view_scroll_y;
	int box_end_x = box_end_map_x * TILE_SIZE - ctx.view.view_scroll_x;
	int box_end_y = box_end_map_y * TILE_SIZE - ctx.view.view_scroll_y;

	static wxColor side_color(0, 0, 0, 200);

	// BatchRenderer doesn't support disabling GL_TEXTURE_2D state globally,
	// but DrawQuad uses white texture if no texture ID is provided or if specific non-textured method used.
	// DrawQuad uses whiteTextureID by default.

	float screen_w = ctx.view.screensize_x * ctx.view.zoom;
	float screen_h = ctx.view.screensize_y * ctx.view.zoom;

	// left side
	if (box_start_map_x >= bounds.start_x) {
		drawFilledRect(ctx.sprite_batch, 0, 0, box_start_x, screen_h, side_color);
	}

	// right side
	if (box_end_map_x < bounds.end_x) {
		drawFilledRect(ctx.sprite_batch, box_end_x, 0, screen_w - box_end_x, screen_h, side_color);
	}

	// top side
	if (box_start_map_y >= bounds.start_y) {
		drawFilledRect(ctx.sprite_batch, box_start_x, 0, box_end_x - box_start_x, box_start_y, side_color);
	}

	// bottom side
	if (box_end_map_y < bounds.end_y) {
		drawFilledRect(ctx.sprite_batch, box_start_x, box_end_y, box_end_x - box_start_x, screen_h - box_end_y, side_color);
	}

	// hidden tiles
	drawRect(ctx.sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxRED);

	// visible tiles
	box_start_x += TILE_SIZE;
	box_start_y += TILE_SIZE;
	box_end_x -= 1 * TILE_SIZE;
	box_end_y -= 1 * TILE_SIZE;
	drawRect(ctx.sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);

	// player position
	box_start_x += (ClientMapWidth - 3) / 2 * TILE_SIZE;
	box_start_y += (ClientMapHeight - 3) / 2 * TILE_SIZE;
	box_end_x = box_start_x + TILE_SIZE;
	box_end_y = box_start_y + TILE_SIZE;
	drawRect(ctx.sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);
}

void GridDrawer::DrawNodeLoadingPlaceholder(const DrawContext& ctx, int nd_map_x, int nd_map_y) {
	int cy = (nd_map_y)*TILE_SIZE - ctx.view.view_scroll_y - ctx.view.getFloorAdjustment();
	int cx = (nd_map_x)*TILE_SIZE - ctx.view.view_scroll_x - ctx.view.getFloorAdjustment();

	glm::vec4 color(1.0f, 0.0f, 1.0f, 0.5f); // 255, 0, 255, 128

	if (g_gui.atlas.ensureAtlasManager()) {
		ctx.sprite_batch.drawRect((float)cx, (float)cy, (float)TILE_SIZE * 4, (float)TILE_SIZE * 4, color, *g_gui.atlas.getAtlasManager());
	}
}

void GridDrawer::drawRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, int width) {
	// glLineWidth(width); // Width ignored for now, BatchRenderer lines are 1px
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.atlas.ensureAtlasManager()) {
		sprite_batch.drawRectLines((float)x, (float)y, (float)w, (float)h, c, *g_gui.atlas.getAtlasManager());
	}
}

void GridDrawer::drawFilledRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color) {
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.atlas.ensureAtlasManager()) {
		sprite_batch.drawRect((float)x, (float)y, (float)w, (float)h, c, *g_gui.atlas.getAtlasManager());
	}
}
