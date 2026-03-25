#ifndef RME_RENDERING_CORE_RENDER_VARIANT_KEY_H_
#define RME_RENDERING_CORE_RENDER_VARIANT_KEY_H_

#include <cstddef>
#include <cstdint>

#include "rendering/core/frame_options.h"
#include "rendering/core/render_settings.h"
#include "rendering/core/render_view.h"

enum class RenderMode : uint8_t {
    Normal = 0,
    Minimap = 1,
    OnlyColors = 2,
};

struct RenderVariantKey {
    RenderMode render_mode = RenderMode::Normal;
    uint8_t zoom_bucket = 0;
    bool show_creatures = false;
    bool show_houses = false;
    bool extended_house_shader = false;
    bool always_show_zones = false;
    bool highlight_locked_doors = false;
    bool show_hooks = false;
    bool show_tooltips = false;
    bool draw_lights = false;
    bool hide_items_when_zoomed = false;
    bool ingame = false;
    bool show_spawns = false;
    bool show_towns = false;
    bool show_items = false;
    bool transparent_items = false;
    bool transparent_floors = false;
    bool show_tech_items = false;
    bool show_special_tiles = false;
    bool show_blocking = false;
    bool highlight_items = false;
    uint32_t current_house_id = 0;

    [[nodiscard]] bool operator==(const RenderVariantKey& other) const = default;

    [[nodiscard]] static RenderVariantKey From(const ViewState& view, const RenderSettings& settings, const FrameOptions& options)
    {
        return RenderVariantKey {
            .render_mode = settings.show_as_minimap ? RenderMode::Minimap : (settings.show_only_colors ? RenderMode::OnlyColors : RenderMode::Normal),
            .zoom_bucket = static_cast<uint8_t>(view.zoom < 10.0f ? 0 : 1),
            .show_creatures = settings.show_creatures,
            .show_houses = settings.show_houses,
            .extended_house_shader = settings.extended_house_shader,
            .always_show_zones = settings.always_show_zones,
            .highlight_locked_doors = settings.highlight_locked_doors,
            .show_hooks = settings.show_hooks,
            .show_tooltips = settings.show_tooltips,
            .draw_lights = settings.isDrawLight(),
            .hide_items_when_zoomed = settings.hide_items_when_zoomed,
            .ingame = settings.ingame,
            .show_spawns = settings.show_spawns,
            .show_towns = settings.show_towns,
            .show_items = settings.show_items,
            .transparent_items = settings.transparent_items,
            .transparent_floors = settings.transparent_floors,
            .show_tech_items = settings.show_tech_items,
            .show_special_tiles = settings.show_special_tiles,
            .show_blocking = settings.show_blocking,
            .highlight_items = settings.highlight_items,
            .current_house_id = options.current_house_id,
        };
    }
};

struct RenderVariantKeyHash {
    [[nodiscard]] size_t operator()(const RenderVariantKey& key) const noexcept
    {
        size_t hash = static_cast<size_t>(key.render_mode);
        hash = (hash * 131U) ^ key.zoom_bucket;
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_creatures);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_houses);
        hash = (hash * 131U) ^ static_cast<size_t>(key.extended_house_shader);
        hash = (hash * 131U) ^ static_cast<size_t>(key.always_show_zones);
        hash = (hash * 131U) ^ static_cast<size_t>(key.highlight_locked_doors);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_hooks);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_tooltips);
        hash = (hash * 131U) ^ static_cast<size_t>(key.draw_lights);
        hash = (hash * 131U) ^ static_cast<size_t>(key.hide_items_when_zoomed);
        hash = (hash * 131U) ^ static_cast<size_t>(key.ingame);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_spawns);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_towns);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_items);
        hash = (hash * 131U) ^ static_cast<size_t>(key.transparent_items);
        hash = (hash * 131U) ^ static_cast<size_t>(key.transparent_floors);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_tech_items);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_special_tiles);
        hash = (hash * 131U) ^ static_cast<size_t>(key.show_blocking);
        hash = (hash * 131U) ^ static_cast<size_t>(key.highlight_items);
        hash = (hash * 131U) ^ static_cast<size_t>(key.current_house_id);
        return hash;
    }
};

#endif
