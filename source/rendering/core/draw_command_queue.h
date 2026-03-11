#ifndef RME_RENDERING_CORE_DRAW_COMMAND_QUEUE_H_
#define RME_RENDERING_CORE_DRAW_COMMAND_QUEUE_H_

#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "app/definitions.h"
#include "rendering/core/tile_render_snapshot.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/utilities/pattern_calculator.h"

struct DrawColorSquareCmd {
    Position pos;
    DrawColor color;
    bool apply_highlight_pulse = false;

    DrawColorSquareCmd() = default;
    DrawColorSquareCmd(Position pos, DrawColor color, bool apply_highlight_pulse) :
        pos(pos), color(color), apply_highlight_pulse(apply_highlight_pulse)
    {
    }
};

struct DrawZoneBrushCmd {
    Position pos;
    ServerItemId sprite_id = 0;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    bool apply_highlight_pulse = false;

    DrawZoneBrushCmd() = default;
    DrawZoneBrushCmd(Position pos, ServerItemId sprite_id, uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool apply_highlight_pulse) :
        pos(pos), sprite_id(sprite_id), r(r), g(g), b(b), a(a), apply_highlight_pulse(apply_highlight_pulse)
    {
    }
};

struct DrawHouseBorderCmd {
    Position pos;
    uint32_t house_id = 0;

    DrawHouseBorderCmd() = default;
    DrawHouseBorderCmd(Position pos, uint32_t house_id) : pos(pos), house_id(house_id) { }
};

struct DrawFilledRectCmd {
    Position pos;
    int width = 0;
    int height = 0;
    DrawColor color;

    DrawFilledRectCmd() = default;
    DrawFilledRectCmd(Position pos, int width, int height, DrawColor color) :
        pos(pos), width(width), height(height), color(color)
    {
    }
};

struct DrawItemCmd {
    Position pos;
    int local_draw_x = 0;
    int local_draw_y = 0;
    ItemRenderSnapshot item;
    SpritePatterns patterns;
    int red = 255;
    int green = 255;
    int blue = 255;
    int alpha = 255;
    bool apply_highlight_pulse = false;

    DrawItemCmd() = default;
    DrawItemCmd(
        Position pos, const ItemRenderSnapshot& item, const SpritePatterns& patterns, int red, int green, int blue, int alpha,
        bool apply_highlight_pulse
    ) :
        DrawItemCmd(pos, 0, 0, item, patterns, red, green, blue, alpha, apply_highlight_pulse)
    {
    }

    DrawItemCmd(
        Position pos, ItemRenderSnapshot&& item, const SpritePatterns& patterns, int red, int green, int blue, int alpha,
        bool apply_highlight_pulse
    ) :
        DrawItemCmd(pos, 0, 0, std::move(item), patterns, red, green, blue, alpha, apply_highlight_pulse)
    {
    }

    DrawItemCmd(
        Position pos, int local_draw_x, int local_draw_y, const ItemRenderSnapshot& item, const SpritePatterns& patterns, int red, int green, int blue, int alpha,
        bool apply_highlight_pulse
    ) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), item(item), patterns(patterns), red(red), green(green), blue(blue), alpha(alpha),
        apply_highlight_pulse(apply_highlight_pulse)
    {
    }

    DrawItemCmd(
        Position pos, int local_draw_x, int local_draw_y, ItemRenderSnapshot&& item, const SpritePatterns& patterns, int red, int green, int blue, int alpha,
        bool apply_highlight_pulse
    ) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), item(std::move(item)), patterns(patterns), red(red), green(green), blue(blue),
        alpha(alpha),
        apply_highlight_pulse(apply_highlight_pulse)
    {
    }
};

struct DrawCreatureCmd {
    Position pos;
    int local_draw_x = 0;
    int local_draw_y = 0;
    CreatureRenderSnapshot creature;
    CreatureDrawOptions options;

    DrawCreatureCmd() = default;
    DrawCreatureCmd(Position pos, const CreatureRenderSnapshot& creature, const CreatureDrawOptions& options) :
        DrawCreatureCmd(pos, 0, 0, creature, options)
    {
    }

    DrawCreatureCmd(Position pos, CreatureRenderSnapshot&& creature, CreatureDrawOptions&& options) :
        DrawCreatureCmd(pos, 0, 0, std::move(creature), std::move(options))
    {
    }

    DrawCreatureCmd(Position pos, int local_draw_x, int local_draw_y, const CreatureRenderSnapshot& creature, const CreatureDrawOptions& options) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), creature(creature), options(options)
    {
    }

    DrawCreatureCmd(Position pos, int local_draw_x, int local_draw_y, CreatureRenderSnapshot&& creature, CreatureDrawOptions&& options) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), creature(std::move(creature)), options(std::move(options))
    {
    }
};

struct DrawMarkerCmd {
    Position pos;
    int local_draw_x = 0;
    int local_draw_y = 0;
    MarkerRenderSnapshot marker;

    DrawMarkerCmd() = default;
    DrawMarkerCmd(Position pos, const MarkerRenderSnapshot& marker) : DrawMarkerCmd(pos, 0, 0, marker) { }
    DrawMarkerCmd(Position pos, MarkerRenderSnapshot&& marker) : DrawMarkerCmd(pos, 0, 0, std::move(marker)) { }

    DrawMarkerCmd(Position pos, int local_draw_x, int local_draw_y, const MarkerRenderSnapshot& marker) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), marker(marker)
    {
    }
    DrawMarkerCmd(Position pos, int local_draw_x, int local_draw_y, MarkerRenderSnapshot&& marker) :
        pos(pos), local_draw_x(local_draw_x), local_draw_y(local_draw_y), marker(std::move(marker))
    {
    }
};

using DrawCommand = std::variant<DrawColorSquareCmd, DrawZoneBrushCmd, DrawHouseBorderCmd, DrawFilledRectCmd, DrawItemCmd, DrawCreatureCmd, DrawMarkerCmd>;

class DrawCommandQueue {
public:
    void clear()
    {
        commands_.clear();
    }

    void reserve(size_t count)
    {
        commands_.reserve(count);
    }

    template<typename Command>
    void push(Command&& command)
    {
        commands_.emplace_back(std::forward<Command>(command));
    }

    void emplaceColorSquare(Position pos, DrawColor color, bool apply_highlight_pulse)
    {
        commands_.emplace_back(std::in_place_type<DrawColorSquareCmd>, pos, color, apply_highlight_pulse);
    }

    void emplaceZoneBrush(Position pos, ServerItemId sprite_id, uint8_t r, uint8_t g, uint8_t b, uint8_t a, bool apply_highlight_pulse)
    {
        commands_.emplace_back(std::in_place_type<DrawZoneBrushCmd>, pos, sprite_id, r, g, b, a, apply_highlight_pulse);
    }

    void emplaceHouseBorder(Position pos, uint32_t house_id)
    {
        commands_.emplace_back(std::in_place_type<DrawHouseBorderCmd>, pos, house_id);
    }

    void emplaceFilledRect(Position pos, int width, int height, DrawColor color)
    {
        commands_.emplace_back(std::in_place_type<DrawFilledRectCmd>, pos, width, height, color);
    }

    void emplaceItem(
        Position pos, int local_draw_x, int local_draw_y, const ItemRenderSnapshot& item, const SpritePatterns& patterns, int red, int green, int blue,
        int alpha,
        bool apply_highlight_pulse
    )
    {
        commands_.emplace_back(
            std::in_place_type<DrawItemCmd>, pos, local_draw_x, local_draw_y, item, patterns, red, green, blue, alpha, apply_highlight_pulse
        );
    }

    void emplaceItem(
        Position pos, int local_draw_x, int local_draw_y, ItemRenderSnapshot&& item, const SpritePatterns& patterns, int red, int green, int blue,
        int alpha,
        bool apply_highlight_pulse
    )
    {
        commands_.emplace_back(
            std::in_place_type<DrawItemCmd>, pos, local_draw_x, local_draw_y, std::move(item), patterns, red, green, blue, alpha, apply_highlight_pulse
        );
    }

    void emplaceCreature(Position pos, int local_draw_x, int local_draw_y, const CreatureRenderSnapshot& creature, const CreatureDrawOptions& options)
    {
        commands_.emplace_back(std::in_place_type<DrawCreatureCmd>, pos, local_draw_x, local_draw_y, creature, options);
    }

    void emplaceCreature(Position pos, int local_draw_x, int local_draw_y, CreatureRenderSnapshot&& creature, CreatureDrawOptions&& options)
    {
        commands_.emplace_back(std::in_place_type<DrawCreatureCmd>, pos, local_draw_x, local_draw_y, std::move(creature), std::move(options));
    }

    void emplaceMarker(Position pos, int local_draw_x, int local_draw_y, const MarkerRenderSnapshot& marker)
    {
        commands_.emplace_back(std::in_place_type<DrawMarkerCmd>, pos, local_draw_x, local_draw_y, marker);
    }

    void emplaceMarker(Position pos, int local_draw_x, int local_draw_y, MarkerRenderSnapshot&& marker)
    {
        commands_.emplace_back(std::in_place_type<DrawMarkerCmd>, pos, local_draw_x, local_draw_y, std::move(marker));
    }

    [[nodiscard]] std::span<const DrawCommand> commands() const
    {
        return commands_;
    }

    [[nodiscard]] size_t size() const
    {
        return commands_.size();
    }

private:
    std::vector<DrawCommand> commands_;
};

#endif
