#include "app/main.h"

// glut include removed

#include "rendering/drawers/tiles/shade_drawer.h"

ShadeDrawer::ShadeDrawer() { }

ShadeDrawer::~ShadeDrawer() { }

#include "rendering/core/atlas_manager.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"

void ShadeDrawer::draw(const DrawContext& ctx)
{
    if (ctx.view.start_z != ctx.view.end_z && ctx.settings.show_shade) {
        glm::vec4 color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);
        float w = ctx.view.screensize_x * ctx.view.zoom;
        float h = ctx.view.screensize_y * ctx.view.zoom;

        ctx.sprite_batch.drawRect(0.0f, 0.0f, w, h, color, ctx.atlas);
    }
}
