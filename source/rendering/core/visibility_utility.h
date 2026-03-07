#ifndef RME_RENDERING_CORE_VISIBILITY_UTILITY_H_
#define RME_RENDERING_CORE_VISIBILITY_UTILITY_H_

#include "app/definitions.h"

namespace rme {

/**
 * Pure visibility logic for tile-based rendering.
 * Operates on raw coordinate data to avoid dependency on ViewState where possible,
 * but designed to be called by ViewState or directly in render loops.
 */
namespace visibility {

inline int calculateLayerOffset(int map_z, int current_floor) {
    if (map_z <= GROUND_LAYER) {
        return (GROUND_LAYER - map_z) * TILE_SIZE;
    }
    return TILE_SIZE * (current_floor - map_z);
}

inline bool isTileVisible(int map_x, int map_y, int map_z,
                        int view_scroll_x, int view_scroll_y,
                        float logical_width, float logical_height,
                        int current_floor,
                        int& out_x, int& out_y) {
    const int offset = calculateLayerOffset(map_z, current_floor);

    out_x = ((map_x * TILE_SIZE) - view_scroll_x) - offset;
    out_y = ((map_y * TILE_SIZE) - view_scroll_y) - offset;
    
    constexpr int MARGIN = PAINTERS_ALGORITHM_SAFETY_MARGIN_PIXELS;

    if (out_x < -MARGIN || out_x > static_cast<int>(logical_width) + MARGIN || 
        out_y < -MARGIN || out_y > static_cast<int>(logical_height) + MARGIN) {
        return false;
    }
    return true;
}

inline bool isPixelVisible(int draw_x, int draw_y, float logical_width, float logical_height, int margin) {
    if (draw_x + TILE_SIZE + margin < 0 || draw_x - margin > static_cast<int>(logical_width) || 
        draw_y + TILE_SIZE + margin < 0 || draw_y - margin > static_cast<int>(logical_height)) {
        return false;
    }
    return true;
}

inline bool isRectVisible(int draw_x, int draw_y, int width, int height, float logical_width, float logical_height, int margin) {
    if (draw_x + width + margin < 0 || draw_x - margin > static_cast<int>(logical_width) || 
        draw_y + height + margin < 0 || draw_y - margin > static_cast<int>(logical_height)) {
        return false;
    }
    return true;
}

inline bool isRectFullyInside(int draw_x, int draw_y, int width, int height, float logical_width, float logical_height) {
    return (draw_x >= 0 && draw_x + width <= static_cast<int>(logical_width) && 
            draw_y >= 0 && draw_y + height <= static_cast<int>(logical_height));
}

inline void getScreenPosition(int map_x, int map_y, int map_z,
                            int view_scroll_x, int view_scroll_y,
                            int current_floor,
                            int& out_x, int& out_y) {
    const int offset = calculateLayerOffset(map_z, current_floor);
    out_x = ((map_x * TILE_SIZE) - view_scroll_x) - offset;
    out_y = ((map_y * TILE_SIZE) - view_scroll_y) - offset;
}

} // namespace visibility
} // namespace rme

#endif
