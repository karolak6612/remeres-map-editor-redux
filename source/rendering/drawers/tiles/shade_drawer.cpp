#include "rendering/drawers/tiles/shade_drawer.h"
#include "app/main.h"


#include "rendering/core/draw_context.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/floor_view_params.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/view_state.h"


ShadeDrawer::ShadeDrawer() {}

ShadeDrawer::~ShadeDrawer() {}

void ShadeDrawer::draw(const DrawContext &ctx,
                       [[maybe_unused]] const FloorViewParams &floor_params) {
  if (ctx.state.view.start_z != ctx.state.view.end_z && ctx.state.options.settings.show_shade) {
    glm::vec4 color(0.0f, 0.0f, 0.0f, 128.0f / 255.0f);
    float w = ctx.state.view.screensize_x * ctx.state.view.zoom;
    float h = ctx.state.view.screensize_y * ctx.state.view.zoom;

    ctx.backend.sprite_batch.drawRect(0.0f, 0.0f, w, h, color,
                              ctx.backend.atlas_manager);
  }
}
