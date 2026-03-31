#include "app/visuals.h"

#include "item_definitions/core/item_definition_store.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/game_sprite.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <array>

namespace {

wxBitmap loadAssetBitmap(std::string_view asset_path) {
	return IMAGE_MANAGER.GetBitmap(asset_path, wxSize(TILE_SIZE, TILE_SIZE), wxNullColour);
}

GameSprite* getSpriteById(uint32_t sprite_id) {
	return dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(sprite_id));
}

GameSprite* getItemSprite(uint16_t item_id) {
	if (const auto definition = g_item_definitions.get(item_id)) {
		return getSpriteById(definition.clientId());
	}
	return nullptr;
}

wxBitmap buildOverlayBitmap(const VisualAppearance& appearance) {
	switch (appearance.type) {
		case VisualAppearanceType::SpriteId:
			if (auto* sprite = getSpriteById(appearance.sprite_id)) {
				return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
			}
			break;
		case VisualAppearanceType::OtherItemVisual:
			if (auto* sprite = getItemSprite(appearance.item_id)) {
				return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			return loadAssetBitmap(appearance.asset_path);
		case VisualAppearanceType::Rgba:
			break;
	}

	return wxNullBitmap;
}

std::vector<std::uint8_t> rgbaPixels(const wxBitmap& bitmap) {
	if (!bitmap.IsOk()) {
		return {};
	}

	const wxImage image = bitmap.ConvertToImage();
	if (!image.IsOk()) {
		return {};
	}

	wxImage scaled = image;
	if (scaled.GetWidth() != TILE_SIZE || scaled.GetHeight() != TILE_SIZE) {
		scaled = scaled.Scale(TILE_SIZE, TILE_SIZE, wxIMAGE_QUALITY_HIGH);
	}
	if (!scaled.HasAlpha()) {
		scaled.InitAlpha();
	}

	std::vector<std::uint8_t> pixels(static_cast<size_t>(TILE_SIZE * TILE_SIZE * 4), 0);
	const auto* source = scaled.GetData();
	const auto* alpha = scaled.GetAlpha();
	for (int index = 0; index < TILE_SIZE * TILE_SIZE; ++index) {
		pixels[static_cast<size_t>(index * 4 + 0)] = source[index * 3 + 0];
		pixels[static_cast<size_t>(index * 4 + 1)] = source[index * 3 + 1];
		pixels[static_cast<size_t>(index * 4 + 2)] = source[index * 3 + 2];
		pixels[static_cast<size_t>(index * 4 + 3)] = alpha ? alpha[index] : 255;
	}
	return pixels;
}

ResolvedVisualResource makeAtlasResource(VisualResourceRegistry& registry, const VisualRule& rule, const wxBitmap& bitmap) {
	const auto pixels = rgbaPixels(bitmap);
	if (pixels.empty()) {
		return {};
	}

	return ResolvedVisualResource {
		.kind = VisualResourceKind::AtlasSprite,
		.color = rule.appearance.color,
		.atlas_sprite_id = registry.AddAtlasSprite(pixels),
		.valid = true
	};
}

ResolvedVisualResource buildRuleResource(VisualResourceRegistry& registry, const VisualRule& rule) {
	switch (rule.appearance.type) {
		case VisualAppearanceType::Rgba:
			return ResolvedVisualResource { .kind = VisualResourceKind::FlatColor, .color = rule.appearance.color, .valid = true };
		case VisualAppearanceType::SpriteId:
			if (rule.match_type == VisualMatchType::Overlay) {
				return makeAtlasResource(registry, rule, buildOverlayBitmap(rule.appearance));
			}
			return ResolvedVisualResource { .kind = VisualResourceKind::NativeSpriteId, .color = rule.appearance.color, .sprite_id = rule.appearance.sprite_id, .valid = rule.appearance.sprite_id != 0 };
		case VisualAppearanceType::OtherItemVisual:
			if (rule.match_type == VisualMatchType::Overlay) {
				return makeAtlasResource(registry, rule, buildOverlayBitmap(rule.appearance));
			}
			return ResolvedVisualResource { .kind = VisualResourceKind::NativeItemVisual, .color = rule.appearance.color, .item_id = rule.appearance.item_id, .valid = rule.appearance.item_id != 0 };
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			return makeAtlasResource(registry, rule, loadAssetBitmap(rule.appearance.asset_path));
	}

	return {};
}

ResolvedVisualResource buildFallbackOverlayResource(VisualResourceRegistry& registry, OverlayVisualKind kind) {
	const auto asset_path = [kind]() -> std::string_view {
		switch (kind) {
			case OverlayVisualKind::DoorLocked:
				return ICON_LOCK;
			case OverlayVisualKind::DoorUnlocked:
				return ICON_LOCK_OPEN;
			case OverlayVisualKind::HookSouth:
				return ICON_ANGLE_UP;
			case OverlayVisualKind::HookEast:
				return ICON_ANGLE_LEFT;
			case OverlayVisualKind::LightIndicator:
				return {};
		}
		return {};
	}();

	if (asset_path.empty()) {
		return {};
	}

	const wxBitmap bitmap = loadAssetBitmap(asset_path);
	const auto pixels = rgbaPixels(bitmap);
	if (pixels.empty()) {
		return {};
	}

	return ResolvedVisualResource {
		.kind = VisualResourceKind::AtlasSprite,
		.color = wxColour(255, 255, 255, 255),
		.atlas_sprite_id = registry.AddAtlasSprite(pixels),
		.valid = true
	};
}

}

void VisualResourceRegistry::Clear() {
	rule_resources_.clear();
	fallback_overlay_resources_.clear();
	atlas_sprites_.clear();
	next_custom_sprite_id_ = 2'800'000;
	uploaded_texture_id_ = 0;
	uploaded_sprite_count_ = 0;
}

void VisualResourceRegistry::SetRuleResource(std::string key, ResolvedVisualResource resource) {
	rule_resources_[std::move(key)] = std::move(resource);
}

const ResolvedVisualResource* VisualResourceRegistry::GetRuleResource(const std::string& key) const {
	if (const auto iterator = rule_resources_.find(key); iterator != rule_resources_.end()) {
		return &iterator->second;
	}
	return nullptr;
}

void VisualResourceRegistry::SetFallbackOverlayResource(OverlayVisualKind kind, ResolvedVisualResource resource) {
	fallback_overlay_resources_[kind] = std::move(resource);
}

const ResolvedVisualResource* VisualResourceRegistry::GetFallbackOverlayResource(OverlayVisualKind kind) const {
	if (const auto iterator = fallback_overlay_resources_.find(kind); iterator != fallback_overlay_resources_.end()) {
		return &iterator->second;
	}
	return nullptr;
}

uint32_t VisualResourceRegistry::AddAtlasSprite(std::vector<std::uint8_t> rgba_pixels) {
	const uint32_t sprite_id = next_custom_sprite_id_++;
	atlas_sprites_.push_back(AtlasSpriteResource { .sprite_id = sprite_id, .rgba_pixels = std::move(rgba_pixels) });
	return sprite_id;
}

void VisualResourceRegistry::EnsureAtlasResourcesUploaded(AtlasManager& atlas) const {
	const uint32_t texture_id = static_cast<uint32_t>(atlas.getTextureId());
	if (texture_id != 0 && uploaded_texture_id_ == texture_id && uploaded_sprite_count_ == atlas_sprites_.size()) {
		return;
	}

	for (const auto& sprite : atlas_sprites_) {
		if (!atlas.hasSprite(sprite.sprite_id)) {
			atlas.addSprite(sprite.sprite_id, sprite.rgba_pixels.data());
		}
	}

	uploaded_texture_id_ = static_cast<uint32_t>(atlas.getTextureId());
	uploaded_sprite_count_ = atlas_sprites_.size();
}

bool Visuals::PrepareRuntimeResources() {
	EnsureServerItemRulesMaterialized();
	resource_registry.Clear();

	for (const auto kind : std::array {
		OverlayVisualKind::DoorLocked,
		OverlayVisualKind::DoorUnlocked,
		OverlayVisualKind::HookSouth,
		OverlayVisualKind::HookEast,
	}) {
		if (const auto fallback = buildFallbackOverlayResource(resource_registry, kind); fallback.valid) {
			resource_registry.SetFallbackOverlayResource(kind, fallback);
		}
	}

	for (const auto& entry : BuildCatalog()) {
		const auto* rule = entry.effective();
		if (!rule || !rule->enabled || !rule->valid) {
			continue;
		}

		if (const auto resource = buildRuleResource(resource_registry, *rule); resource.valid) {
			resource_registry.SetRuleResource(rule->key, resource);
		}
	}

	runtime_resources_dirty = false;
	return true;
}

void Visuals::EnsureRuntimeResourcesPrepared() const {
	if (!runtime_resources_dirty) {
		return;
	}

	const_cast<Visuals*>(this)->PrepareRuntimeResources();
}
