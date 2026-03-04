//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_METADATA_H_
#define RME_RENDERING_CORE_SPRITE_METADATA_H_

#include "rendering/core/animator.h"
#include "rendering/core/sprite_light.h"
#include <cstdint>
#include <memory>
#include <utility>

struct SpriteMetadata {
    uint32_t id = 0;
    uint8_t width = 0;
    uint8_t height = 0;
    uint8_t layers = 0;
    uint8_t pattern_x = 0;
    uint8_t pattern_y = 0;
    uint8_t pattern_z = 0;
    uint8_t frames = 0;
    uint32_t numsprites = 0;

    uint16_t draw_height = 0;
    uint16_t drawoffset_x = 0;
    uint16_t drawoffset_y = 0;

    uint16_t minimap_color = 0;

    bool has_light = false;
    SpriteLight light;

    bool is_simple = false;

    std::unique_ptr<Animator> animator;

    void updateSimpleStatus()
    {
        is_simple = (numsprites == 1 && frames == 1 && layers == 1 && width == 1 && height == 1);
    }

    [[nodiscard]] std::pair<int, int> getDrawOffset() const
    {
        return {drawoffset_x, drawoffset_y};
    }

    [[nodiscard]] size_t getIndex(int w, int h, int layer, int px, int py, int pz, int frame) const
    {
        if (is_simple || frames == 0) {
            return 0;
        }
        size_t idx = (frames > 1) ? frame % frames : 0;
        idx = idx * static_cast<size_t>(pattern_z) + static_cast<size_t>(pz);
        idx = idx * static_cast<size_t>(pattern_y) + static_cast<size_t>(py);
        idx = idx * static_cast<size_t>(pattern_x) + static_cast<size_t>(px);
        idx = idx * static_cast<size_t>(layers) + static_cast<size_t>(layer);
        idx = idx * static_cast<size_t>(height) + static_cast<size_t>(h);
        idx = idx * static_cast<size_t>(width) + static_cast<size_t>(w);
        return idx;
    }
};

#endif // RME_RENDERING_CORE_SPRITE_METADATA_H_
