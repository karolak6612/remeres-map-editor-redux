#include "rendering/drawers/overlays/visual_overlay_drawer.h"

#include "app/visuals.h"
#include "rendering/core/render_view.h"
#include "util/image_manager.h"

#include <nanovg.h>

void VisualOverlayDrawer::add(VisualOverlayRequest request) {
	requests.push_back(std::move(request));
}

void VisualOverlayDrawer::clear() {
	requests.clear();
}

void VisualOverlayDrawer::draw(NVGcontext* vg, const RenderView& view) {
	if (!vg || requests.empty()) {
		return;
	}

	nvgSave(vg);

	for (const auto& request : requests) {
		if (request.pos.z != view.floor) {
			continue;
		}

		int unscaled_x = 0;
		int unscaled_y = 0;
		if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const int image_id = IMAGE_MANAGER.GetNanoVGImage(vg, request.asset_path, Visuals::EffectiveImageTint(request.color));
		if (image_id == 0) {
			continue;
		}

		const float zoom = view.zoom;
		const float tile_size = 32.0f / zoom;
		float x = unscaled_x / zoom;
		float y = unscaled_y / zoom;
		float w = tile_size;
		float h = tile_size;

		switch (request.placement) {
			case VisualOverlayPlacement::TileFill:
				break;
			case VisualOverlayPlacement::TileInset: {
				const float inset = tile_size * 0.08f;
				x += inset;
				y += inset;
				w -= inset * 2.0f;
				h -= inset * 2.0f;
				break;
			}
			case VisualOverlayPlacement::TileCenter: {
				const float size = tile_size * 0.7f;
				x += (tile_size - size) * 0.5f;
				y += (tile_size - size) * 0.5f;
				w = size;
				h = size;
				break;
			}
		}

		NVGpaint paint = nvgImagePattern(vg, x, y, w, h, 0.0f, image_id, request.color.Alpha() / 255.0f);
		nvgBeginPath(vg);
		nvgRect(vg, x, y, w, h);
		nvgFillPaint(vg, paint);
		nvgFill(vg);
	}

	nvgRestore(vg);
}
