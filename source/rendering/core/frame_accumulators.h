#ifndef RME_RENDERING_CORE_FRAME_ACCUMULATORS_H_
#define RME_RENDERING_CORE_FRAME_ACCUMULATORS_H_

#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include "rendering/ui/tooltip_collector.h"
#include <vector>

// Groups all per-frame accumulation buffers into a single struct.
// Ensures atomic clear-at-frame-start and consistent lifecycle.
// Owned by MapDrawer; passed by reference through DrawContext so that
// tile-level code can append tooltips, hooks, doors, and creature names
// without holding pointers to individual collectors.
struct FrameAccumulators {
    TooltipCollector tooltips;
    std::vector<HookIndicatorDrawer::HookRequest> hooks;
    std::vector<DoorIndicatorDrawer::DoorRequest> doors;
    std::vector<CreatureLabel> creature_names;

    void clear()
    {
        tooltips.clear();
        hooks.clear();
        doors.clear();
        creature_names.clear();
    }

    void reserve(size_t hook_cap, size_t door_cap, size_t creature_cap, size_t tooltip_cap = 0)
    {
        if (tooltip_cap > 0) {
            tooltips.reserve(tooltip_cap);
        }
        hooks.reserve(hook_cap);
        doors.reserve(door_cap);
        creature_names.reserve(creature_cap);
    }
};

#endif
