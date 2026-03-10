#include "rendering/drawers/overlays/door_indicator_drawer.h"
#include <nanovg.h>
#include "rendering/core/render_view.h"
#include "rendering/utilities/icon_renderer.h"
#include "util/image_manager.h"

DoorIndicatorDrawer::DoorIndicatorDrawer() = default;

DoorIndicatorDrawer::~DoorIndicatorDrawer() = default;

void DoorIndicatorDrawer::draw(NVGcontext* vg, const ViewState& view, std::span<const DoorRequest> doors) {
	if (doors.empty() || !vg) {
		return;
	}

	nvgSave(vg);

	const NVGcolor colorLocked = nvgRGBA(255, 0, 0, 255); // Red
	const NVGcolor colorUnlocked = nvgRGBA(102, 255, 0, 255); // Green (#66ff00)

	const float zoomFactor = 1.0f / view.zoom;
	const float iconSize = 12.0f * zoomFactor;
	const float outlineOffset = 1.0f * zoomFactor;

	for (const auto& request : doors) {
		// Only render doors on the current floor
		if (request.pos.z != view.floor) {
			continue;
		}

		auto vis = view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z);
		if (!vis) {
			continue;
		}

		const float zoom = view.zoom;
		const float x = vis->x / zoom;
		const float y = vis->y / zoom;
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
