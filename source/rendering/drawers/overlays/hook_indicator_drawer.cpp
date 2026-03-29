#include "rendering/drawers/overlays/hook_indicator_drawer.h"

#include "app/visuals.h"
#include "item_definitions/core/item_definition_store.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/render_view.h"
#include "rendering/utilities/icon_renderer.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <nanovg.h>

namespace {
wxBitmap buildOverlayBitmap(const VisualAppearance& appearance) {
	switch (appearance.type) {
		case VisualAppearanceType::SpriteId:
			if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(appearance.sprite_id))) {
				return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
			}
			break;
		case VisualAppearanceType::OtherItemVisual:
			if (const auto definition = g_item_definitions.get(appearance.item_id)) {
				if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(definition.clientId()))) {
					return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
				}
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (!appearance.asset_path.empty()) {
				return IMAGE_MANAGER.GetBitmap(appearance.asset_path, wxSize(48, 48), Visuals::EffectiveImageTint(appearance.color));
			}
			break;
		case VisualAppearanceType::Rgba:
			break;
	}

	return wxNullBitmap;
}

int resolveOverlayImage(NVGcontext* vg, const VisualRule& rule, std::string_view cache_key) {
	switch (rule.appearance.type) {
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (!rule.appearance.asset_path.empty()) {
				return IMAGE_MANAGER.GetNanoVGImage(vg, rule.appearance.asset_path, Visuals::EffectiveImageTint(rule.appearance.color));
			}
			break;
		case VisualAppearanceType::SpriteId:
		case VisualAppearanceType::OtherItemVisual:
			if (const wxBitmap bitmap = buildOverlayBitmap(rule.appearance); bitmap.IsOk()) {
				return IMAGE_MANAGER.GetNanoVGImage(vg, bitmap, cache_key);
			}
			break;
		case VisualAppearanceType::Rgba:
			break;
	}

	return 0;
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

void HookIndicatorDrawer::draw(NVGcontext* vg, const RenderView& view) {
	if (requests.empty() || !vg) {
		return;
	}

	nvgSave(vg);

	const float zoomFactor = 1.0f / view.zoom;
	const float iconSize = 24.0f * zoomFactor;
	const float outlineOffset = 1.0f * zoomFactor;

	for (const auto& request : requests) {
		if (request.pos.z != view.floor) {
			continue;
		}

		int unscaled_x = 0;
		int unscaled_y = 0;
		if (!view.IsTileVisible(request.pos.x, request.pos.y, request.pos.z, unscaled_x, unscaled_y)) {
			continue;
		}

		const float zoom = view.zoom;
		const float x = unscaled_x / zoom;
		const float y = unscaled_y / zoom;
		const float tile_size = 32.0f / zoom;

		auto draw_request = [&](OverlayVisualKind kind, float px, float py, std::string_view fallback_icon) {
			const VisualRule* rule = g_visuals.ResolveOverlay(kind);
			const wxColour color_value = rule ? rule->appearance.color : wxColour(204, 255, 0, 255);
			const NVGcolor color = nvgRGBA(color_value.Red(), color_value.Green(), color_value.Blue(), color_value.Alpha());

			if (rule) {
				const std::string cache_key = "overlay.hook." + std::string(kind == OverlayVisualKind::HookSouth ? "south" : "east") + "." + std::to_string(static_cast<int>(rule->appearance.type)) + "." + std::to_string(rule->appearance.sprite_id) + "." + std::to_string(rule->appearance.item_id) + "." + rule->appearance.asset_path;
				const int image_id = resolveOverlayImage(vg, *rule, cache_key);
				if (image_id != 0) {
					const float image_size = iconSize;
					NVGpaint paint = nvgImagePattern(vg, px - image_size / 2.0f, py - image_size / 2.0f, image_size, image_size, 0.0f, image_id, color_value.Alpha() / 255.0f);
					nvgBeginPath(vg);
					nvgRect(vg, px - image_size / 2.0f, py - image_size / 2.0f, image_size, image_size);
					nvgFillPaint(vg, paint);
					nvgFill(vg);
					return;
				}
			}

			IconRenderer::DrawIconWithBorder(vg, px, py, iconSize, outlineOffset, fallback_icon, color);
		};

		if (request.south) {
			draw_request(OverlayVisualKind::HookSouth, x, y + tile_size / 2.0f, ICON_ANGLE_UP);
		}
		if (request.east) {
			draw_request(OverlayVisualKind::HookEast, x + tile_size / 2.0f, y, ICON_ANGLE_LEFT);
		}
	}

	nvgRestore(vg);
}
