#include "app/main.h"

#include "rendering/drawers/cursors/live_cursor_drawer.h"

#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/core/sprite_batch.h"
#include "editor/editor.h"
#include "live/live_socket.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/sprite_database.h"
#include "rendering/core/atlas_lifecycle.h"
#include "rendering/core/texture_gc.h"
#include "rendering/io/sprite_loader.h"
#include "ui/gui.h"

void LiveCursorDrawer::draw(const DrawContext& ctx, Editor& editor) {
	if (ctx.options.ingame || !editor.live_manager.IsLive()) {
		return;
	}

	LiveSocket& live = editor.live_manager.GetSocket();
	for (const LiveCursor& cursor : live.getCursorList()) {
		if (cursor.pos.z <= GROUND_LAYER && ctx.view.floor > GROUND_LAYER) {
			continue;
		}

		if (cursor.pos.z > GROUND_LAYER && ctx.view.floor <= (GROUND_LAYER + 1)) {
			continue;
		}

		wxColor draw_color = cursor.color;
		if (cursor.pos.z < ctx.view.floor) {
			draw_color = wxColor(
				draw_color.Red(),
				draw_color.Green(),
				draw_color.Blue(),
				std::max<uint8_t>(draw_color.Alpha() / 2, 64)
			);
		}

		int offset = ctx.view.CalculateLayerOffset(cursor.pos.z);

		float draw_x = ((cursor.pos.x * TILE_SIZE) - ctx.view.view_scroll_x) - offset;
		float draw_y = ((cursor.pos.y * TILE_SIZE) - ctx.view.view_scroll_y) - offset;

		glm::vec4 color(
			draw_color.Red() / 255.0f,
			draw_color.Green() / 255.0f,
			draw_color.Blue() / 255.0f,
			draw_color.Alpha() / 255.0f
		);

		if (g_gui.atlas.ensureAtlasManager()) {
			ctx.sprite_batch.drawRect(draw_x, draw_y, (float)TILE_SIZE, (float)TILE_SIZE, color, *g_gui.atlas.getAtlasManager());
		}
	}
}
