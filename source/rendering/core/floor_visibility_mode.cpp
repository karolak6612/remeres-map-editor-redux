#include "rendering/core/floor_visibility_mode.h"

// This translation unit intentionally contains compile-time checks only.
// Floor visibility range selection is constexpr, so these assertions document
// the expected map/minimap rules and make accidental behavior changes fail at build time.
static_assert(SanitizeFloorVisibilityMode(0) == FloorVisibilityMode::ClientVisible);
static_assert(SanitizeFloorVisibilityMode(1) == FloorVisibilityMode::AllVisible);
static_assert(SanitizeFloorVisibilityMode(-1) == FloorVisibilityMode::ClientVisible);
static_assert(SanitizeFloorVisibilityMode(2) == FloorVisibilityMode::ClientVisible);

static_assert(BuildFloorVisibilityRange(7, false, FloorVisibilityMode::AllVisible).start_floor == 7);
static_assert(BuildFloorVisibilityRange(7, false, FloorVisibilityMode::AllVisible).end_floor == 7);
static_assert(!BuildFloorVisibilityRange(7, false, FloorVisibilityMode::AllVisible).draw_all_visited_floors);

static_assert(BuildFloorVisibilityRange(7, true, FloorVisibilityMode::ClientVisible).start_floor == GROUND_LAYER);
static_assert(BuildFloorVisibilityRange(7, true, FloorVisibilityMode::ClientVisible).end_floor == 0);
static_assert(!BuildFloorVisibilityRange(7, true, FloorVisibilityMode::ClientVisible).draw_all_visited_floors);

static_assert(BuildFloorVisibilityRange(10, true, FloorVisibilityMode::ClientVisible).start_floor == 12);
static_assert(BuildFloorVisibilityRange(10, true, FloorVisibilityMode::ClientVisible).end_floor == 8);
static_assert(!BuildFloorVisibilityRange(10, true, FloorVisibilityMode::ClientVisible).draw_all_visited_floors);

static_assert(BuildFloorVisibilityRange(10, true, FloorVisibilityMode::AllVisible).start_floor == MAP_MAX_LAYER);
static_assert(BuildFloorVisibilityRange(10, true, FloorVisibilityMode::AllVisible).end_floor == 10);
static_assert(BuildFloorVisibilityRange(10, true, FloorVisibilityMode::AllVisible).draw_all_visited_floors);
static_assert(BuildFloorVisibilityRange(0, true, FloorVisibilityMode::AllVisible).start_floor == MAP_MAX_LAYER);
static_assert(BuildFloorVisibilityRange(0, true, FloorVisibilityMode::AllVisible).end_floor == 0);
static_assert(BuildFloorVisibilityRange(15, true, FloorVisibilityMode::AllVisible).start_floor == MAP_MAX_LAYER);
static_assert(BuildFloorVisibilityRange(15, true, FloorVisibilityMode::AllVisible).end_floor == MAP_MAX_LAYER);
