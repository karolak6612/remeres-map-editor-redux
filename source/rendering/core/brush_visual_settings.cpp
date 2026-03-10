#include "rendering/core/brush_visual_settings.h"
#include "app/settings.h"

BrushVisualSettings BrushVisualSettings::FromSettings(const Settings& settings) {
    BrushVisualSettings s;
    s.cursor_red = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_RED));
    s.cursor_green = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_GREEN));
    s.cursor_blue = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_BLUE));
    s.cursor_alpha = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_ALPHA));
    s.cursor_alt_red = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_ALT_RED));
    s.cursor_alt_green = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_ALT_GREEN));
    s.cursor_alt_blue = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_ALT_BLUE));
    s.cursor_alt_alpha = static_cast<uint8_t>(settings.getInteger(Config::CURSOR_ALT_ALPHA));
    s.use_automagic = settings.getBoolean(Config::USE_AUTOMAGIC);
    return s;
}
