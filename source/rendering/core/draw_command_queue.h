#ifndef RME_RENDERING_CORE_DRAW_COMMAND_QUEUE_H_
#define RME_RENDERING_CORE_DRAW_COMMAND_QUEUE_H_

#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "app/definitions.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/utilities/pattern_calculator.h"

class Creature;
class Tile;
class Waypoint;

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

struct DrawItemCmd {
    int draw_x = 0;
    int draw_y = 0;
    BlitItemParams params;
    SpritePatterns patterns;

    DrawItemCmd(int draw_x, int draw_y, BlitItemParams params, SpritePatterns patterns) :
        draw_x(draw_x), draw_y(draw_y), params(std::move(params)), patterns(std::move(patterns))
    {
    }
};

struct DrawCreatureCmd {
    int draw_x = 0;
    int draw_y = 0;
    const Creature* creature = nullptr;
    CreatureDrawOptions options;
};

struct DrawMarkerCmd {
    int draw_x = 0;
    int draw_y = 0;
    Tile* tile = nullptr;
    Waypoint* waypoint = nullptr;
    uint32_t current_house_id = 0;
};

using DrawCommand = std::variant<DrawColorSquareCmd, DrawZoneBrushCmd, DrawHouseBorderCmd, DrawItemCmd, DrawCreatureCmd, DrawMarkerCmd>;

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
