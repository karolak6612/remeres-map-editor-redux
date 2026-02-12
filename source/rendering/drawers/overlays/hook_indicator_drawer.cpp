#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include <nanovg.h>
#include "rendering/core/render_view.h"
#include "app/definitions.h"
#include "util/image_manager.h"
#include "rendering/utilities/icon_renderer.h"

HookIndicatorDrawer::HookIndicatorDrawer() {
	requests.reserve(100);
}

HookIndicatorDrawer::~HookIndicatorDrawer() = default;

void HookIndicatorDrawer::addHook(const Position& pos, bool south, bool east) {
	requests.push_back({ pos, south, east });
}

void HookIndicatorDrawer::clear() {
	requests.clear();
}

void HookIndicatorDrawer::draw(NVGcontext* vg, const RenderView& view) {
	if (requests.empty() || !vg) {
		return;
	}

	nvgSave(vg);

	// Style
	const NVGcolor tintColor = nvgRGBA(204, 255, 0, 255); // Fluorescent Yellow-Green (#ccff00)

	const float zoomFactor = 1.0f / view.zoom;
	const float iconSize = 24.0f * zoomFactor;
	const float outlineOffset = 1.0f * zoomFactor;

	for (const auto& request : requests) {
		// Only render hooks on the current floor
		if (request.pos.z != view.floor) {
			continue;
		}

		int unscaled_x, unscaled_y;
		if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const float zoom = view.zoom;
		const float x = unscaled_x / zoom;
		const float y = unscaled_y / zoom;
		const float tileSize = 32.0f / zoom;

		if (request.south) {
			// Center of WEST border, pointing NORTH (towards corner)
			IconRenderer::DrawIconWithBorder(vg, x, y + tileSize / 2.0f, iconSize, outlineOffset, ICON_ANGLE_UP, tintColor);
		}

		if (request.east) {
			// Center of NORTH border, pointing WEST (towards corner)
			IconRenderer::DrawIconWithBorder(vg, x + tileSize / 2.0f, y, iconSize, outlineOffset, ICON_ANGLE_LEFT, tintColor);
		}
	}

	nvgRestore(vg);
}
