#include "rendering/drawers/overlays/door_indicator_drawer.h"

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

void drawDoorRequest(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view, const DoorIndicatorDrawer::DoorRequest& request) {
	int unscaled_x = 0;
	int unscaled_y = 0;
	if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
		return;
	}

	const OverlayVisualKind kind = request.locked ? OverlayVisualKind::DoorLocked : OverlayVisualKind::DoorUnlocked;
	const wxColour fallback_color = request.locked ? wxColour(255, 0, 0, 255) : wxColour(102, 255, 0, 255);
	const auto* resource = g_visuals.ResolveOverlayResource(kind);
	const auto* fallback_resource = g_visuals.GetFallbackOverlayResource(kind);
	if (!fallback_resource || fallback_resource->atlas_sprite_id == 0) {
		return;
	}

	const wxColour tint = resource && resource->kind == VisualResourceKind::FlatColor ? resource->color : resource ? resource->color : fallback_color;
	const uint32_t sprite_id = resource && resource->kind == VisualResourceKind::AtlasSprite ? resource->atlas_sprite_id : fallback_resource->atlas_sprite_id;
	const float x = static_cast<float>(unscaled_x);
	const float y = static_cast<float>(unscaled_y);
	const float tile_size = static_cast<float>(TILE_SIZE);
	const float icon_size = 12.0f;

	const auto draw_at = [&](float px, float py) {
		drawIcon(sprite_batch, atlas_manager, sprite_id, px, py, icon_size * 1.4f, tint);
	};

	if (request.south) {
		draw_at(x, y + tile_size * 0.5f);
	}
	if (request.east) {
		draw_at(x + tile_size * 0.5f, y);
	}
	if (!request.south && !request.east) {
		draw_at(x + tile_size * 0.5f, y + tile_size * 0.5f);
	}
}

}

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

void DoorIndicatorDrawer::draw(SpriteBatch& sprite_batch, const AtlasManager& atlas_manager, const RenderView& view) const {
	for (const auto& request : requests) {
		if (request.pos.z == view.floor) {
			drawDoorRequest(sprite_batch, atlas_manager, view, request);
		}
	}
}
