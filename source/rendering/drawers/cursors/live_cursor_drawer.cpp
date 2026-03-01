#include "app/main.h"

#include "rendering/drawers/cursors/live_cursor_drawer.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/sprite_batch.h"
#include "editor/editor.h"
#include "live/live_socket.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"

void LiveCursorDrawer::draw(const DrawContext& ctx, Editor& editor) {
	if (ctx.options.ingame || !editor.live_manager.IsLive()) {
		return;
	}

	LiveSocket& live = editor.live_manager.GetSocket();
	for (LiveCursor& cursor : live.getCursorList()) {
		if (cursor.pos.z <= GROUND_LAYER && ctx.view.floor > GROUND_LAYER) {
			continue;
		}

		if (cursor.pos.z > GROUND_LAYER && ctx.view.floor <= 8) {
			continue;
		}

		if (cursor.pos.z < ctx.view.floor) {
			cursor.color = wxColor(
				cursor.color.Red(),
				cursor.color.Green(),
				cursor.color.Blue(),
				std::max<uint8_t>(cursor.color.Alpha() / 2, 64)
			);
		}

		int offset;
		if (cursor.pos.z <= GROUND_LAYER) {
			offset = (GROUND_LAYER - cursor.pos.z) * TILE_SIZE;
		} else {
			offset = TILE_SIZE * (ctx.view.floor - cursor.pos.z);
		}

		float draw_x = ((cursor.pos.x * TILE_SIZE) - ctx.view.view_scroll_x) - offset;
		float draw_y = ((cursor.pos.y * TILE_SIZE) - ctx.view.view_scroll_y) - offset;

		glm::vec4 color(
			cursor.color.Red() / 255.0f,
			cursor.color.Green() / 255.0f,
			cursor.color.Blue() / 255.0f,
			cursor.color.Alpha() / 255.0f
		);

		if (g_gui.gfx.ensureAtlasManager()) {
			ctx.sprite_batch.drawRect(draw_x, draw_y, (float)TILE_SIZE, (float)TILE_SIZE, color, *g_gui.gfx.getAtlasManager());
		}
	}
}
