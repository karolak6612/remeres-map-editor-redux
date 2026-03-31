#include "rendering/drawers/overlays/visual_overlay_drawer.h"

#include "rendering/core/atlas_manager.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"

void VisualOverlayDrawer::add(VisualOverlayRequest request) {
	requests.push_back(std::move(request));
}

void VisualOverlayDrawer::clear() {
	requests.clear();
}

void VisualOverlayDrawer::draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const {
	if (requests.empty()) {
		return;
	}

	for (const auto& request : requests) {
		if (request.pos.z != view.floor || request.atlas_sprite_id == 0) {
			continue;
		}

		const AtlasRegion* region = atlas_manager.getRegion(request.atlas_sprite_id);
		if (!region) {
			continue;
		}

		int unscaled_x = 0;
		int unscaled_y = 0;
		if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const float zoom = view.zoom;
		const float tile_size = 32.0f / zoom;
		float x = unscaled_x / zoom;
		float y = unscaled_y / zoom;
		float width = tile_size;
		float height = tile_size;

		switch (request.placement) {
			case VisualOverlayPlacement::TileFill:
				break;
			case VisualOverlayPlacement::TileInset: {
				const float inset = tile_size * 0.08f;
				x += inset;
				y += inset;
				width -= inset * 2.0f;
				height -= inset * 2.0f;
				break;
			}
			case VisualOverlayPlacement::TileCenter: {
				const float size = tile_size * 0.7f;
				x += (tile_size - size) * 0.5f;
				y += (tile_size - size) * 0.5f;
				width = size;
				height = size;
				break;
			}
		}

		sprite_batch.draw(
			x,
			y,
			width,
			height,
			*region,
			request.color.Red() / 255.0f,
			request.color.Green() / 255.0f,
			request.color.Blue() / 255.0f,
			request.color.Alpha() / 255.0f
		);
	}
}
