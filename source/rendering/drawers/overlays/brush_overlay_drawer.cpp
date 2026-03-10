//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

// glut include removed

#include "app/definitions.h"
#include "editor/editor.h"
#include "game/creatures.h"
#include "game/outfit.h"
#include "game/sprites.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/draw_context.h"
#include "rendering/core/frame_options.h"
#include "rendering/core/primitive_renderer.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/drawers/cursors/brush_cursor_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/overlays/brush_overlay_drawer.h"

#include "brushes/brush.h"

#include "brushes/border/optional_border_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/doodad/doodad_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"
#include "brushes/house/house_brush.h"
#include "brushes/house/house_exit_brush.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/spawn/spawn_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/waypoint/waypoint_brush.h"

#include "brushes/waypoint/waypoint_brush.h"

// Helper to get color from brush visual settings
glm::vec4 BrushOverlayDrawer::get_brush_color(BrushColor color, const BrushVisualSettings& visual)
{
    glm::vec4 c(1.0f);
    switch (color) {
        case COLOR_BRUSH:
            c = glm::vec4(
                visual.cursor_red / 255.0f, visual.cursor_green / 255.0f, visual.cursor_blue / 255.0f, visual.cursor_alpha / 255.0f
            );
            break;

        case COLOR_FLAG_BRUSH:
        case COLOR_HOUSE_BRUSH:
            c = glm::vec4(
                visual.cursor_alt_red / 255.0f, visual.cursor_alt_green / 255.0f, visual.cursor_alt_blue / 255.0f,
                visual.cursor_alt_alpha / 255.0f
            );
            break;

        case COLOR_SPAWN_BRUSH:
        case COLOR_ERASER:
        case COLOR_INVALID:
            c = glm::vec4(166.0f / 255.0f, 0.0f, 0.0f, 128.0f / 255.0f);
            break;

        case COLOR_VALID:
            c = glm::vec4(0.0f, 166.0f / 255.0f, 0.0f, 128.0f / 255.0f);
            break;

        default:
            c = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
            break;
    }
    return c;
}

glm::vec4 BrushOverlayDrawer::get_check_color(Brush* brush, Editor& editor, const Position& pos, const BrushVisualSettings& visual)
{
    if (brush->canDraw(&editor.map, pos)) {
        return get_brush_color(COLOR_VALID, visual);
    } else {
        return get_brush_color(COLOR_INVALID, visual);
    }
}

BrushOverlayDrawer::BrushOverlayDrawer() { }

BrushOverlayDrawer::~BrushOverlayDrawer() { }

void BrushOverlayDrawer::draw(const DrawContext& ctx, const BrushOverlayContext& overlay)
{
    if (!overlay.is_drawing_mode) {
        return;
    }
    if (!overlay.current_brush) {
        return;
    }
    if (ctx.settings.ingame) {
        return;
    }

    const auto& visual = *overlay.visual;
    auto* editor = overlay.editor;
    Brush* brush = overlay.current_brush;

    BrushColor brushColorType = COLOR_BLANK;
    if (brush->is<TerrainBrush>() || brush->is<TableBrush>() || brush->is<CarpetBrush>()) {
        brushColorType = COLOR_BRUSH;
    } else if (brush->is<HouseBrush>()) {
        brushColorType = COLOR_HOUSE_BRUSH;
    } else if (brush->is<FlagBrush>()) {
        brushColorType = COLOR_FLAG_BRUSH;
    } else if (brush->is<SpawnBrush>()) {
        brushColorType = COLOR_SPAWN_BRUSH;
    } else if (brush->is<EraserBrush>()) {
        brushColorType = COLOR_ERASER;
    }

    glm::vec4 brushColor = get_brush_color(brushColorType, visual);

    if (overlay.is_dragging_draw) {
        drawDragging(
            ctx, visual, overlay.item_drawer, overlay.sprite_drawer, *editor, brush, brushColor, overlay.brush_shape,
            overlay.last_click_map_x, overlay.last_click_map_y
        );
    } else {
        drawStationary(
            ctx, visual, overlay.item_drawer, overlay.sprite_drawer, overlay.creature_drawer, overlay.brush_cursor_drawer, *editor,
            brush, brushColor, overlay.brush_shape, overlay.brush_size
        );
    }
}

void BrushOverlayDrawer::drawDragging(
    const DrawContext& ctx, const BrushVisualSettings& visual, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, Editor& editor,
    Brush* brush, const glm::vec4& brushColor, BrushShape brush_shape, int last_click_map_x, int last_click_map_y
)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;
    const auto& atlas = ctx.atlas;

    ASSERT(brush->canDrag());

    if (brush->is<WallBrush>()) {
        int last_click_start_map_x = std::min(last_click_map_x, view.mouse_map_x);
        int last_click_start_map_y = std::min(last_click_map_y, view.mouse_map_y);
        int last_click_end_map_x = std::max(last_click_map_x, view.mouse_map_x) + 1;
        int last_click_end_map_y = std::max(last_click_map_y, view.mouse_map_y) + 1;

        int last_click_start_sx = last_click_start_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
        int last_click_start_sy = last_click_start_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
        int last_click_end_sx = last_click_end_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
        int last_click_end_sy = last_click_end_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

        int delta_x = last_click_end_sx - last_click_start_sx;
        int delta_y = last_click_end_sy - last_click_start_sy;

        // Top
        sprite_batch.drawRect(
            static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy),
            static_cast<float>(last_click_end_sx - last_click_start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas
        );

        // Bottom
        if (delta_y > TILE_SIZE) {
            sprite_batch.drawRect(
                static_cast<float>(last_click_start_sx), static_cast<float>(last_click_end_sy - TILE_SIZE),
                static_cast<float>(last_click_end_sx - last_click_start_sx), static_cast<float>(TILE_SIZE), brushColor, atlas
            );
        }

        // Right
        if (delta_x > TILE_SIZE && delta_y > TILE_SIZE) {
            float h = (last_click_end_sy - TILE_SIZE) - (last_click_start_sy + TILE_SIZE);
            sprite_batch.drawRect(
                static_cast<float>(last_click_end_sx - TILE_SIZE), static_cast<float>(last_click_start_sy + TILE_SIZE),
                static_cast<float>(TILE_SIZE), h, brushColor, atlas
            );
        }

        // Left
        if (delta_y > TILE_SIZE) {
            float h = (last_click_end_sy - TILE_SIZE) - (last_click_start_sy + TILE_SIZE);
            sprite_batch.drawRect(
                static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE),
                h, brushColor, atlas
            );
        }
        return;
    }

    if (brush_shape == BRUSHSHAPE_SQUARE || brush->is<SpawnBrush>()) {
        if (brush->is<RAWBrush>() || brush->is<OptionalBorderBrush>()) {
            int start_x, end_x;
            int start_y, end_y;

            if (view.mouse_map_x < last_click_map_x) {
                start_x = view.mouse_map_x;
                end_x = last_click_map_x;
            } else {
                start_x = last_click_map_x;
                end_x = view.mouse_map_x;
            }
            if (view.mouse_map_y < last_click_map_y) {
                start_y = view.mouse_map_y;
                end_y = last_click_map_y;
            } else {
                start_y = last_click_map_y;
                end_y = view.mouse_map_y;
            }

            RAWBrush* raw_brush = nullptr;
            if (brush->is<RAWBrush>()) {
                raw_brush = brush->as<RAWBrush>();
            }

            for (int y = start_y; y <= end_y; y++) {
                int cy = y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
                for (int x = start_x; x <= end_x; x++) {
                    int cx = x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
                    if (brush->is<OptionalBorderBrush>()) {
                        sprite_batch.drawRect(
                            static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE),
                            get_check_color(brush, editor, Position(x, y, view.floor), visual), atlas
                        );
                    } else {
                        item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
                    }
                }
            }
        } else {
            int last_click_start_map_x = std::min(last_click_map_x, view.mouse_map_x);
            int last_click_start_map_y = std::min(last_click_map_y, view.mouse_map_y);
            int last_click_end_map_x = std::max(last_click_map_x, view.mouse_map_x) + 1;
            int last_click_end_map_y = std::max(last_click_map_y, view.mouse_map_y) + 1;

            int last_click_start_sx = last_click_start_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
            int last_click_start_sy = last_click_start_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
            int last_click_end_sx = last_click_end_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
            int last_click_end_sy = last_click_end_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

            float w = last_click_end_sx - last_click_start_sx;
            float h = last_click_end_sy - last_click_start_sy;
            bool autoborder_active = visual.use_automagic && brush->needBorders();
            if (autoborder_active) {
                // Draw outline only
                float thickness = 1.0f; // Thin border

                // Top
                sprite_batch.drawRect(
                    static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy), w, thickness, brushColor, atlas
                );
                // Bottom
                sprite_batch.drawRect(
                    static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + h - thickness), w, thickness,
                    brushColor, atlas
                );
                // Left
                sprite_batch.drawRect(
                    static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy + thickness), thickness,
                    h - 2 * thickness, brushColor, atlas
                );
                // Right
                sprite_batch.drawRect(
                    static_cast<float>(last_click_start_sx + w - thickness), static_cast<float>(last_click_start_sy + thickness), thickness,
                    h - 2 * thickness, brushColor, atlas
                );
            } else {
                sprite_batch.drawRect(
                    static_cast<float>(last_click_start_sx), static_cast<float>(last_click_start_sy), w, h, brushColor, atlas
                );
            }
        }
    } else if (brush_shape == BRUSHSHAPE_CIRCLE) {
        // Calculate drawing offsets
        int start_x, end_x;
        int start_y, end_y;
        int width = std::max(
            std::abs(std::max(view.mouse_map_y, last_click_map_y) - std::min(view.mouse_map_y, last_click_map_y)),
            std::abs(std::max(view.mouse_map_x, last_click_map_x) - std::min(view.mouse_map_x, last_click_map_x))
        );

        if (view.mouse_map_x < last_click_map_x) {
            start_x = last_click_map_x - width;
            end_x = last_click_map_x;
        } else {
            start_x = last_click_map_x;
            end_x = last_click_map_x + width;
        }

        if (view.mouse_map_y < last_click_map_y) {
            start_y = last_click_map_y - width;
            end_y = last_click_map_y;
        } else {
            start_y = last_click_map_y;
            end_y = last_click_map_y + width;
        }

        int center_x = start_x + (end_x - start_x) / 2;
        int center_y = start_y + (end_y - start_y) / 2;
        float radii = width / 2.0f + 0.005f;

        RAWBrush* raw_brush = nullptr;
        if (brush->is<RAWBrush>()) {
            raw_brush = brush->as<RAWBrush>();
        }

        for (int y = start_y - 1; y <= end_y + 1; y++) {
            int cy = y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
            float dy = center_y - y;
            for (int x = start_x - 1; x <= end_x + 1; x++) {
                int cx = x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();

                float dx = center_x - x;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance < radii) {
                    if (brush->is<RAWBrush>()) {
                        item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
                    } else {
                        sprite_batch.drawRect(
                            static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE),
                            brushColor, atlas
                        );
                    }
                }
            }
        }
    }
}

void BrushOverlayDrawer::drawStationary(
    const DrawContext& ctx, const BrushVisualSettings& visual, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer,
    CreatureDrawer* creature_drawer, BrushCursorDrawer* brush_cursor_drawer, Editor& editor, Brush* brush,
    const glm::vec4& brushColor, BrushShape brush_shape, int brush_size
)
{
    if (brush->is<WallBrush>()) {
        drawStationaryWall(ctx, brush, brushColor, brush_size);
    } else if (brush->is<DoorBrush>()) {
        drawStationaryDoor(ctx, brush, editor, visual);
    } else if (brush->is<CreatureBrush>()) {
        drawStationaryCreature(ctx, sprite_drawer, creature_drawer, brush, editor);
    } else if (!brush->is<DoodadBrush>()) {
        drawStationaryGeneric(ctx, item_drawer, sprite_drawer, brush_cursor_drawer, editor, brush, brushColor, brush_shape, brush_size, visual);
    }
}

void BrushOverlayDrawer::drawStationaryWall(const DrawContext& ctx, Brush* brush, const glm::vec4& brushColor, int brush_size)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;
    const auto& atlas = ctx.atlas;

    int start_map_x = view.mouse_map_x - brush_size;
    int start_map_y = view.mouse_map_y - brush_size;
    int end_map_x = view.mouse_map_x + brush_size + 1;
    int end_map_y = view.mouse_map_y + brush_size + 1;

    int start_sx = start_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
    int start_sy = start_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
    int end_sx = end_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
    int end_sy = end_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

    int delta_x = end_sx - start_sx;
    int delta_y = end_sy - start_sy;

    // Top
    sprite_batch.drawRect(
        static_cast<float>(start_sx), static_cast<float>(start_sy), static_cast<float>(end_sx - start_sx), static_cast<float>(TILE_SIZE),
        brushColor, atlas
    );

    // Bottom
    if (delta_y > TILE_SIZE) {
        sprite_batch.drawRect(
            static_cast<float>(start_sx), static_cast<float>(end_sy - TILE_SIZE), static_cast<float>(end_sx - start_sx),
            static_cast<float>(TILE_SIZE), brushColor, atlas
        );
    }

    // Right
    if (delta_x > TILE_SIZE && delta_y > TILE_SIZE) {
        float h = static_cast<float>(end_sy - start_sy - 2 * TILE_SIZE);
        sprite_batch.drawRect(
            static_cast<float>(end_sx - TILE_SIZE), static_cast<float>(start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor,
            atlas
        );
    }

    // Left
    if (delta_y > TILE_SIZE) {
        float h = static_cast<float>(end_sy - start_sy - 2 * TILE_SIZE);
        sprite_batch.drawRect(
            static_cast<float>(start_sx), static_cast<float>(start_sy + TILE_SIZE), static_cast<float>(TILE_SIZE), h, brushColor, atlas
        );
    }
}

void BrushOverlayDrawer::drawStationaryDoor(const DrawContext& ctx, Brush* brush, Editor& editor, const BrushVisualSettings& visual)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;

    int cx = view.mouse_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
    int cy = view.mouse_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

    sprite_batch.drawRect(
        static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE),
        get_check_color(brush, editor, Position(view.mouse_map_x, view.mouse_map_y, view.floor), visual), ctx.atlas
    );
}

void BrushOverlayDrawer::drawStationaryCreature(
    const DrawContext& ctx, SpriteDrawer* sprite_drawer, CreatureDrawer* creature_drawer, Brush* brush, Editor& editor
)
{
    auto& sprite_batch = ctx.sprite_batch;
    const auto& view = ctx.view;

    int cx = view.mouse_map_x * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
    int cy = view.mouse_map_y * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();

    CreatureBrush* creature_brush = brush->as<CreatureBrush>();
    if (creature_brush->canDraw(&editor.map, Position(view.mouse_map_x, view.mouse_map_y, view.floor))) {
        creature_drawer->BlitCreature(
            sprite_batch, sprite_drawer, cx, cy, creature_brush->getType()->outfit, SOUTH,
            CreatureDrawOptions {.color = DrawColor(255, 255, 255, 160)}
        );
    } else {
        creature_drawer->BlitCreature(
            sprite_batch, sprite_drawer, cx, cy, creature_brush->getType()->outfit, SOUTH,
            CreatureDrawOptions {.color = DrawColor(255, 64, 64, 160)}
        );
    }
}

void BrushOverlayDrawer::drawStationaryGeneric(
    const DrawContext& ctx, ItemDrawer* item_drawer, SpriteDrawer* sprite_drawer, BrushCursorDrawer* brush_cursor_drawer, Editor& editor,
    Brush* brush, const glm::vec4& brushColor, BrushShape brush_shape, int brush_size, const BrushVisualSettings& visual
)
{
    auto& sprite_batch = ctx.sprite_batch;
    auto& primitive_renderer = ctx.primitive_renderer;
    const auto& view = ctx.view;
    const auto& atlas = ctx.atlas;

    RAWBrush* raw_brush = nullptr;
    if (brush->is<RAWBrush>()) {
        raw_brush = brush->as<RAWBrush>();
    }

    for (int y = -brush_size - 1; y <= brush_size + 1; y++) {
        int cy = (view.mouse_map_y + y) * TILE_SIZE - view.view_scroll_y - view.getFloorAdjustment();
        for (int x = -brush_size - 1; x <= brush_size + 1; x++) {
            int cx = (view.mouse_map_x + x) * TILE_SIZE - view.view_scroll_x - view.getFloorAdjustment();
            if (brush_shape == BRUSHSHAPE_SQUARE) {
                if (x >= -brush_size && x <= brush_size && y >= -brush_size && y <= brush_size) {
                    if (brush->is<RAWBrush>()) {
                        item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
                    } else {
                        if (brush->is<WaypointBrush>()) {
                            uint8_t r, g, b;
                            get_color(brush, editor, Position(view.mouse_map_x + x, view.mouse_map_y + y, view.floor), r, g, b);
                            brush_cursor_drawer->draw(sprite_batch, primitive_renderer, atlas, cx, cy, brush, r, g, b);
                        } else {
                            glm::vec4 c = brushColor;
                            if (brush->is<HouseExitBrush>() || brush->is<OptionalBorderBrush>()) {
                                c = get_check_color(
                                    brush, editor, Position(view.mouse_map_x + x, view.mouse_map_y + y, view.floor), visual
                                );
                            }
                            sprite_batch.drawRect(
                                static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE),
                                static_cast<float>(TILE_SIZE), c, atlas
                            );
                        }
                    }
                }
            } else if (brush_shape == BRUSHSHAPE_CIRCLE) {
                double distance = sqrt(double(x * x) + double(y * y));
                if (distance < brush_size + 0.005) {
                    if (brush->is<RAWBrush>()) {
                        item_drawer->DrawRawBrush(sprite_batch, sprite_drawer, cx, cy, raw_brush->getItemID(), 160, 160, 160, 160);
                    } else {
                        if (brush->is<WaypointBrush>()) {
                            uint8_t r, g, b;
                            get_color(brush, editor, Position(view.mouse_map_x + x, view.mouse_map_y + y, view.floor), r, g, b);
                            brush_cursor_drawer->draw(sprite_batch, primitive_renderer, atlas, cx, cy, brush, r, g, b);
                        } else {
                            glm::vec4 c = brushColor;
                            if (brush->is<HouseExitBrush>() || brush->is<OptionalBorderBrush>()) {
                                c = get_check_color(
                                    brush, editor, Position(view.mouse_map_x + x, view.mouse_map_y + y, view.floor), visual
                                );
                            }
                            sprite_batch.drawRect(
                                static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(TILE_SIZE),
                                static_cast<float>(TILE_SIZE), c, atlas
                            );
                        }
                    }
                }
            }
        }
    }
}

void BrushOverlayDrawer::get_color(Brush* brush, Editor& editor, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b)
{
    if (brush->canDraw(&editor.map, position)) {
        if (brush->is<WaypointBrush>()) {
            r = 0x00;
            g = 0xff, b = 0x00;
        } else {
            r = 0x00;
            g = 0x00, b = 0xff;
        }
    } else {
        r = 0xff;
        g = 0x00, b = 0x00;
    }
}
