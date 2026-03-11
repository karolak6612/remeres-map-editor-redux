#ifndef RME_RENDERING_CORE_BRUSH_VISUAL_SETTINGS_H_
#define RME_RENDERING_CORE_BRUSH_VISUAL_SETTINGS_H_

#include <cstdint>

class Settings;

// Visual settings that control how the brush cursor and brush overlay are drawn.
// Separated from RenderSettings because they are brush-UI concerns, not render-pipeline concerns.
struct BrushVisualSettings {
    uint8_t cursor_red = 0;
    uint8_t cursor_green = 166;
    uint8_t cursor_blue = 0;
    uint8_t cursor_alpha = 128;

    uint8_t cursor_alt_red = 0;
    uint8_t cursor_alt_green = 166;
    uint8_t cursor_alt_blue = 0;
    uint8_t cursor_alt_alpha = 128;

    bool use_automagic = true;

    // Factory: populate from a settings object.
    static BrushVisualSettings FromSettings(const Settings& settings);
};

#endif
