#include "rendering/drawers/overlays/hook_indicator_drawer.h"
#include <nanovg.h>
#include "rendering/core/render_view.h"
#include "app/definitions.h"

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
	const NVGcolor fillColor = nvgRGBA(255, 215, 0, 240); // Golden Yellow, more intense
	const NVGcolor strokeColor = nvgRGBA(0, 0, 0, 255); // Solid black border

	const float zoomFactor = 1.0f / view.zoom;
	const float arrowSize = 15.0f * zoomFactor; // Slightly larger
	const float headSize = 6.0f * zoomFactor;
	const float shaftWidth = 4.0f * zoomFactor; // Thicker shaft to show more fill
	const float strokeWidth = 1.0f * zoomFactor; // Thinner border relative to size

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
			// "Up arrow" on center of WEST border, pointing NORTH (towards corner)
			float cx = x;
			float cy = y + tileSize / 2.0f;

			nvgBeginPath(vg);
			// Shaft
			nvgRect(vg, cx - shaftWidth / 2.0f, cy, shaftWidth, -(arrowSize - headSize));

			// Head
			nvgMoveTo(vg, cx - headSize, cy - (arrowSize - headSize));
			nvgLineTo(vg, cx, cy - arrowSize);
			nvgLineTo(vg, cx + headSize, cy - (arrowSize - headSize));
			nvgClosePath(vg);

			nvgFillColor(vg, fillColor);
			nvgFill(vg);
			nvgStrokeColor(vg, strokeColor);
			nvgStrokeWidth(vg, strokeWidth);
			nvgStroke(vg);
		}

		if (request.east) {
			// "Left arrow" on center of NORTH border, pointing WEST (towards corner)
			float cx = x + tileSize / 2.0f;
			float cy = y;

			nvgBeginPath(vg);
			// Shaft
			nvgRect(vg, cx, cy - shaftWidth / 2.0f, -(arrowSize - headSize), shaftWidth);

			// Head
			nvgMoveTo(vg, cx - (arrowSize - headSize), cy - headSize);
			nvgLineTo(vg, cx - arrowSize, cy);
			nvgLineTo(vg, cx - (arrowSize - headSize), cy + headSize);
			nvgClosePath(vg);

			nvgFillColor(vg, fillColor);
			nvgFill(vg);
			nvgStrokeColor(vg, strokeColor);
			nvgStrokeWidth(vg, strokeWidth);
			nvgStroke(vg);
		}
	}

	nvgRestore(vg);
}
