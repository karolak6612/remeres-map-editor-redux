#include "rendering/drawers/overlays/hook_indicator_drawer.h"

#include "app/visuals.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/render_view.h"
#include "rendering/core/sprite_batch.h"

namespace {

void drawIcon(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, uint32_t sprite_id, float center_x, float center_y, float size, const wxColour& color) {
	if (const AtlasRegion* region = atlas_manager.getRegion(sprite_id)) {
		sprite_batch.draw(
			center_x - size * 0.5f,
			center_y - size * 0.5f,
			size,
			size,
			*region,
			color.Red() / 255.0f,
			color.Green() / 255.0f,
			color.Blue() / 255.0f,
			color.Alpha() / 255.0f
		);
	}
}

void drawHookIcon(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view, const HookIndicatorDrawer::HookRequest& request, OverlayVisualKind kind, float center_x, float center_y) {
	const auto* resource = g_visuals.ResolveOverlayResource(kind);
	const auto* fallback_resource = g_visuals.GetFallbackOverlayResource(kind);
	if (!fallback_resource || fallback_resource->atlas_sprite_id == 0) {
		return;
	}

	const wxColour tint = resource && resource->kind == VisualResourceKind::FlatColor ? resource->color : resource ? resource->color : wxColour(204, 255, 0, 255);
	const uint32_t sprite_id = resource && resource->kind == VisualResourceKind::AtlasSprite ? resource->atlas_sprite_id : fallback_resource->atlas_sprite_id;
	const float size = 24.0f / view.zoom;
	drawIcon(sprite_batch, atlas_manager, sprite_id, center_x, center_y, size, tint);
}

}

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

void HookIndicatorDrawer::draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const {
	for (const auto& request : requests) {
		if (request.pos.z != view.floor) {
			continue;
		}

		int unscaled_x = 0;
		int unscaled_y = 0;
		if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const float x = static_cast<float>(unscaled_x);
		const float y = static_cast<float>(unscaled_y);
		const float tile_size = static_cast<float>(TILE_SIZE);

		if (request.south) {
			drawHookIcon(sprite_batch, atlas_manager, view, request, OverlayVisualKind::HookSouth, x, y + tile_size * 0.5f);
		}
		if (request.east) {
			drawHookIcon(sprite_batch, atlas_manager, view, request, OverlayVisualKind::HookEast, x + tile_size * 0.5f, y);
		}
	}
}
