#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include <nanovg.h>
#include "rendering/core/draw_context.h"
#include "rendering/core/view_state.h"
#include "rendering/utilities/icon_renderer.h"
#include "util/image_manager.h"

DoorIndicatorDrawer::DoorIndicatorDrawer() {
	requests.reserve(100);
}

DoorIndicatorDrawer::~DoorIndicatorDrawer() = default;

void DoorIndicatorDrawer::addDoor(const Position& pos, bool locked, bool south, bool east) {
	requests.push_back({ pos, locked, south, east });
}

void DoorIndicatorDrawer::clear() {
	requests.clear();
}

void DoorIndicatorDrawer::draw(NVGcontext* vg, const DrawContext& ctx) {
	if (requests.empty() || !vg) {
		return;
	}

	nvgSave(vg);

	const NVGcolor colorLocked = nvgRGBA(255, 0, 0, 255); // Red
	const NVGcolor colorUnlocked = nvgRGBA(102, 255, 0, 255); // Green (#66ff00)

	const float zoomFactor = 1.0f / ctx.view.zoom;
	const float iconSize = 12.0f * zoomFactor;
	const float outlineOffset = 1.0f * zoomFactor;

	for (const auto& request : requests) {
		// Only render doors on the current floor
		if (request.pos.z != ctx.view.floor) {
			continue;
		}

		int unscaled_x, unscaled_y;
		if (!ctx.view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const float zoom = ctx.view.zoom;
		const float x = unscaled_x / zoom;
		const float y = unscaled_y / zoom;
		const float TILE_SIZE = 32.0f / zoom;

		const std::string_view icon = request.locked ? ICON_LOCK : ICON_LOCK_OPEN;
		const NVGcolor color = request.locked ? colorLocked : colorUnlocked;

		if (request.south) {
			// Center of WEST border
			IconRenderer::DrawIconWithBorder(vg, x, y + TILE_SIZE / 2.0f, iconSize, outlineOffset, icon, color);
		}
		if (request.east) {
			// Center of NORTH border
			IconRenderer::DrawIconWithBorder(vg, x + TILE_SIZE / 2.0f, y, iconSize, outlineOffset, icon, color);
		}

		if (!request.south && !request.east) {
			// Center of TILE
			IconRenderer::DrawIconWithBorder(vg, x + TILE_SIZE / 2.0f, y + TILE_SIZE / 2.0f, iconSize, outlineOffset, icon, color);
		}
	}

	nvgRestore(vg);
}
