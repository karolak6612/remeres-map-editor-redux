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
	const NVGcolor fillColor = nvgRGBA(0, 120, 215, 200); // Standard RME blue accent
	const NVGcolor strokeColor = nvgRGBA(255, 255, 255, 100); // Subtle white outline

	const float arrowSize = 10.0f; // Fixed pixel size as requested
	const float headSize = 4.0f;
	const float shaftWidth = 2.0f;

	for (const auto& request : requests) {
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
			nvgStrokeWidth(vg, 1.0f);
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
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}
	}

	nvgRestore(vg);
}
