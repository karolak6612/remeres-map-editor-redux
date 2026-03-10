#include "app/main.h"

#include "rendering/drawers/cursors/live_cursor_drawer.h"

#include "editor/editor.h"
#include "live/live_socket.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"

void LiveCursorDrawer::draw(const DrawContext& ctx, Editor& editor)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;
    const auto& settings = ctx.settings;

    if (settings.ingame || !editor.live_manager.IsLive()) {
        return;
    }

    LiveSocket& live = editor.live_manager.GetSocket();
    for (LiveCursor& cursor : live.getCursorList()) {
        if (cursor.pos.z <= GROUND_LAYER && view.floor > GROUND_LAYER) {
            continue;
        }

        if (cursor.pos.z > GROUND_LAYER && view.floor <= 8) {
            continue;
        }

        if (cursor.pos.z < view.floor) {
            cursor.color
                = wxColor(cursor.color.Red(), cursor.color.Green(), cursor.color.Blue(), std::max<uint8_t>(cursor.color.Alpha() / 2, 64));
        }

        int offset;
        if (cursor.pos.z <= GROUND_LAYER) {
            offset = (GROUND_LAYER - cursor.pos.z) * TILE_SIZE;
        } else {
            offset = TILE_SIZE * (view.floor - cursor.pos.z);
        }

        float draw_x = ((cursor.pos.x * TILE_SIZE) - view.view_scroll_x) - offset;
        float draw_y = ((cursor.pos.y * TILE_SIZE) - view.view_scroll_y) - offset;

        glm::vec4 color(
            cursor.color.Red() / 255.0f, cursor.color.Green() / 255.0f, cursor.color.Blue() / 255.0f, cursor.color.Alpha() / 255.0f
        );

        sprite_batch.drawRect(draw_x, draw_y, (float)TILE_SIZE, (float)TILE_SIZE, color, ctx.atlas);
    }
}
