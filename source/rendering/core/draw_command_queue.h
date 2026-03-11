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
    int draw_x = 0;
    int draw_y = 0;
    DrawColor color;
};

struct DrawZoneBrushCmd {
    int draw_x = 0;
    int draw_y = 0;
    ServerItemId sprite_id = 0;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

struct DrawHouseBorderCmd {
    int draw_x = 0;
    int draw_y = 0;
    DrawColor color;
};

struct DrawFilledRectCmd {
    int draw_x = 0;
    int draw_y = 0;
    int width = 0;
    int height = 0;
    DrawColor color;
};

struct DrawItemCmd {
    int draw_x = 0;
    int draw_y = 0;
    ItemRenderSnapshot item;
    SpritePatterns patterns;
    int red = 255;
    int green = 255;
    int blue = 255;
    int alpha = 255;
};

struct DrawCreatureCmd {
    int draw_x = 0;
    int draw_y = 0;
    CreatureRenderSnapshot creature;
    CreatureDrawOptions options;
};

struct DrawMarkerCmd {
    int draw_x = 0;
    int draw_y = 0;
    MarkerRenderSnapshot marker;
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
