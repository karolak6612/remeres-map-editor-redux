#include "app/visuals.h"

#include <ranges>

#include <wx/colour.h>
#include <wx/filename.h>

namespace {

std::string normalize(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

std::string pathExtension(const std::string& path) {
	return normalize(wxFileName::FileName(path).GetExt().ToStdString());
}

}

VisualRule Visuals::MakeItemRule(uint16_t item_id) {
	VisualRule rule;
	rule.match_type = VisualMatchType::ItemId;
	rule.match_id = item_id;
	rule.key = MakeKeyForItemId(item_id);
	rule.label = DeriveLabel(rule);
	rule.group = DeriveGroup(rule);
	rule.appearance.type = VisualAppearanceType::OtherItemVisual;
	rule.appearance.item_id = item_id;
	rule.appearance.color = wxColour(255, 255, 255, 255);
	return rule;
}

VisualRule Visuals::MakeMarkerRule(MarkerVisualKind kind) {
	VisualRule rule;
	rule.match_type = VisualMatchType::Marker;
	rule.match_value = ToString(kind);
	rule.key = MakeKeyForMarker(kind);
	rule.label = DeriveLabel(rule);
	rule.group = DeriveGroup(rule);
	return rule;
}

VisualRule Visuals::MakeOverlayRule(OverlayVisualKind kind) {
	VisualRule rule;
	rule.match_type = VisualMatchType::Overlay;
	rule.match_value = ToString(kind);
	rule.key = MakeKeyForOverlay(kind);
	rule.label = DeriveLabel(rule);
	rule.group = DeriveGroup(rule);
	return rule;
}

VisualRule Visuals::MakeTileRule(TileVisualKind kind) {
	VisualRule rule;
	rule.match_type = VisualMatchType::Tile;
	rule.match_value = ToString(kind);
	rule.key = MakeKeyForTile(kind);
	rule.label = DeriveLabel(rule);
	rule.group = DeriveGroup(rule);
	return rule;
}

wxColour Visuals::CombineColor(const wxColour& base_color, const wxColour& tint) {
	return wxColour(
		static_cast<unsigned char>(base_color.Red() * tint.Red() / 255),
		static_cast<unsigned char>(base_color.Green() * tint.Green() / 255),
		static_cast<unsigned char>(base_color.Blue() * tint.Blue() / 255),
		static_cast<unsigned char>(base_color.Alpha() * tint.Alpha() / 255)
	);
}

wxColour Visuals::EffectiveImageTint(const wxColour& tint) {
	if (!tint.IsOk()) {
		return {};
	}

	if (tint.Red() == 255 && tint.Green() == 255 && tint.Blue() == 255) {
		return {};
	}

	return tint;
}

bool Visuals::SupportsSpriteModes(const VisualRule& rule) {
	return rule.match_type == VisualMatchType::ItemId
		|| rule.match_type == VisualMatchType::ClientId
		|| rule.match_type == VisualMatchType::Marker
		|| rule.match_type == VisualMatchType::Overlay
		|| rule.match_type == VisualMatchType::Tile;
}

bool Visuals::SupportsImageModes(const VisualRule& rule) {
	return rule.match_type == VisualMatchType::ItemId
		|| rule.match_type == VisualMatchType::ClientId
		|| rule.match_type == VisualMatchType::Marker
		|| rule.match_type == VisualMatchType::Overlay
		|| rule.match_type == VisualMatchType::Tile;
}

std::string Visuals::BuildKey(const VisualRule& rule) {
	switch (rule.match_type) {
		case VisualMatchType::ItemId:
			return MakeKeyForItemId(static_cast<uint16_t>(rule.match_id));
		case VisualMatchType::ClientId:
			break;
		case VisualMatchType::Marker:
			if (const auto kind = ParseMarkerKind(rule.match_value); kind.has_value()) {
				return MakeKeyForMarker(*kind);
			}
			break;
		case VisualMatchType::Overlay:
			if (const auto kind = ParseOverlayKind(rule.match_value); kind.has_value()) {
				return MakeKeyForOverlay(*kind);
			}
			break;
		case VisualMatchType::Tile:
			if (const auto kind = ParseTileKind(rule.match_value); kind.has_value()) {
				return MakeKeyForTile(*kind);
			}
			break;
	}
	return {};
}

std::string Visuals::DeriveLabel(const VisualRule& rule) {
	switch (rule.match_type) {
		case VisualMatchType::ItemId:
			return "Item ID " + std::to_string(rule.match_id);
		case VisualMatchType::ClientId:
			return "Legacy Client Visual ID " + std::to_string(rule.match_id);
		case VisualMatchType::Marker:
			return "Marker: " + rule.match_value;
		case VisualMatchType::Overlay:
			return "Overlay: " + rule.match_value;
		case VisualMatchType::Tile:
			return "Tile: " + rule.match_value;
	}
	return "Visual Rule";
}

std::string Visuals::DeriveGroup(const VisualRule& rule) {
	switch (rule.match_type) {
		case VisualMatchType::ItemId:
		case VisualMatchType::ClientId:
			return "Items";
		case VisualMatchType::Marker:
			return "Markers";
		case VisualMatchType::Overlay:
			return "Overlays";
		case VisualMatchType::Tile:
			return "Tile/House/Zone";
	}
	return "Visuals";
}

void Visuals::ValidateRule(VisualRule& rule) {
	rule.valid = true;
	rule.validation_error.clear();

	if (rule.key.empty()) {
		rule.valid = false;
		rule.validation_error = "Rule key is empty.";
		return;
	}

	switch (rule.match_type) {
		case VisualMatchType::ItemId:
		case VisualMatchType::ClientId:
			if (rule.match_id <= 0) {
				rule.valid = false;
				rule.validation_error = "Numeric match id must be greater than 0.";
				return;
			}
			break;
		case VisualMatchType::Marker:
			if (!ParseMarkerKind(rule.match_value).has_value()) {
				rule.valid = false;
				rule.validation_error = "Unknown marker kind.";
				return;
			}
			break;
		case VisualMatchType::Overlay:
			if (!ParseOverlayKind(rule.match_value).has_value()) {
				rule.valid = false;
				rule.validation_error = "Unknown overlay kind.";
				return;
			}
			break;
		case VisualMatchType::Tile:
			if (!ParseTileKind(rule.match_value).has_value()) {
				rule.valid = false;
				rule.validation_error = "Unknown tile kind.";
				return;
			}
			break;
	}

	switch (rule.appearance.type) {
		case VisualAppearanceType::Rgba:
			break;
		case VisualAppearanceType::SpriteId:
			if (!SupportsSpriteModes(rule) || rule.appearance.sprite_id == 0) {
				rule.valid = false;
				rule.validation_error = "SPR sprite appearance requires a valid sprite id.";
			}
			break;
		case VisualAppearanceType::OtherItemVisual:
			if (!SupportsSpriteModes(rule) || rule.appearance.item_id == 0) {
				rule.valid = false;
				rule.validation_error = "Other item visual requires a valid target item id.";
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (!SupportsImageModes(rule) || rule.appearance.asset_path.empty()) {
				rule.valid = false;
				rule.validation_error = "Image appearance requires a file path.";
			} else {
				wxFileName image_path(wxString::FromUTF8(rule.appearance.asset_path));
				if (!image_path.IsAbsolute()) {
					rule.valid = false;
					rule.validation_error = "Image path must be absolute.";
					break;
				}
				if (!image_path.FileExists()) {
					rule.valid = false;
					rule.validation_error = "Image file does not exist.";
					break;
				}
				const std::string expected_extension = rule.appearance.type == VisualAppearanceType::Png ? "png" : "svg";
				if (const std::string extension = pathExtension(rule.appearance.asset_path); !extension.empty() && extension != expected_extension) {
					rule.valid = false;
					rule.validation_error = "Image path extension does not match the selected appearance type.";
				}
			}
			break;
	}
}

std::string Visuals::ToString(VisualMatchType type) {
	switch (type) {
		case VisualMatchType::ItemId:
			return "item_id";
		case VisualMatchType::ClientId:
			return "client_id";
		case VisualMatchType::Marker:
			return "marker";
		case VisualMatchType::Overlay:
			return "overlay";
		case VisualMatchType::Tile:
			return "tile";
	}
	return "item_id";
}

std::string Visuals::ToString(MarkerVisualKind kind) {
	switch (kind) {
		case MarkerVisualKind::Waypoint:
			return "waypoint";
		case MarkerVisualKind::HouseExitCurrent:
			return "house_exit_current";
		case MarkerVisualKind::HouseExitOther:
			return "house_exit_other";
		case MarkerVisualKind::TownTemple:
			return "town_temple";
		case MarkerVisualKind::Spawn:
			return "spawn";
		case MarkerVisualKind::SpawnSelected:
			return "spawn_selected";
	}
	return "waypoint";
}

std::string Visuals::ToString(OverlayVisualKind kind) {
	switch (kind) {
		case OverlayVisualKind::DoorLocked:
			return "door_locked";
		case OverlayVisualKind::DoorUnlocked:
			return "door_unlocked";
		case OverlayVisualKind::HookSouth:
			return "hook_south";
		case OverlayVisualKind::HookEast:
			return "hook_east";
		case OverlayVisualKind::LightIndicator:
			return "light_indicator";
	}
	return "door_locked";
}

std::string Visuals::ToString(TileVisualKind kind) {
	switch (kind) {
		case TileVisualKind::Blocking:
			return "blocking";
		case TileVisualKind::Pz:
			return "pz";
		case TileVisualKind::Pvp:
			return "pvp";
		case TileVisualKind::NoLogout:
			return "no_logout";
		case TileVisualKind::NoPvp:
			return "no_pvp";
		case TileVisualKind::HouseOverlay:
			return "house_overlay";
	}
	return "blocking";
}

std::string Visuals::ToString(VisualAppearanceType type) {
	switch (type) {
		case VisualAppearanceType::Rgba:
			return "rgba";
		case VisualAppearanceType::SpriteId:
			return "sprite_id";
		case VisualAppearanceType::OtherItemVisual:
			return "other_item_visual";
		case VisualAppearanceType::Png:
			return "png";
		case VisualAppearanceType::Svg:
			return "svg";
	}
	return "rgba";
}

std::optional<VisualMatchType> Visuals::ParseMatchType(const std::string& value) {
	const std::string normalized = normalize(value);
	if (normalized == "item_id") return VisualMatchType::ItemId;
	if (normalized == "client_id") return VisualMatchType::ClientId;
	if (normalized == "marker") return VisualMatchType::Marker;
	if (normalized == "overlay") return VisualMatchType::Overlay;
	if (normalized == "tile") return VisualMatchType::Tile;
	return std::nullopt;
}

std::optional<MarkerVisualKind> Visuals::ParseMarkerKind(const std::string& value) {
	const std::string normalized = normalize(value);
	if (normalized == "waypoint") return MarkerVisualKind::Waypoint;
	if (normalized == "house_exit_current") return MarkerVisualKind::HouseExitCurrent;
	if (normalized == "house_exit_other") return MarkerVisualKind::HouseExitOther;
	if (normalized == "town_temple") return MarkerVisualKind::TownTemple;
	if (normalized == "spawn") return MarkerVisualKind::Spawn;
	if (normalized == "spawn_selected") return MarkerVisualKind::SpawnSelected;
	return std::nullopt;
}

std::optional<OverlayVisualKind> Visuals::ParseOverlayKind(const std::string& value) {
	const std::string normalized = normalize(value);
	if (normalized == "door_locked") return OverlayVisualKind::DoorLocked;
	if (normalized == "door_unlocked") return OverlayVisualKind::DoorUnlocked;
	if (normalized == "hook_south") return OverlayVisualKind::HookSouth;
	if (normalized == "hook_east") return OverlayVisualKind::HookEast;
	if (normalized == "light_indicator") return OverlayVisualKind::LightIndicator;
	return std::nullopt;
}

std::optional<TileVisualKind> Visuals::ParseTileKind(const std::string& value) {
	const std::string normalized = normalize(value);
	if (normalized == "blocking") return TileVisualKind::Blocking;
	if (normalized == "pz") return TileVisualKind::Pz;
	if (normalized == "pvp") return TileVisualKind::Pvp;
	if (normalized == "no_logout") return TileVisualKind::NoLogout;
	if (normalized == "no_pvp") return TileVisualKind::NoPvp;
	if (normalized == "house_overlay") return TileVisualKind::HouseOverlay;
	return std::nullopt;
}

std::optional<VisualAppearanceType> Visuals::ParseAppearanceType(const std::string& value) {
	const std::string normalized = normalize(value);
	if (normalized == "rgba") return VisualAppearanceType::Rgba;
	if (normalized == "sprite_id") return VisualAppearanceType::SpriteId;
	if (normalized == "other_item_visual") return VisualAppearanceType::OtherItemVisual;
	if (normalized == "png") return VisualAppearanceType::Png;
	if (normalized == "svg") return VisualAppearanceType::Svg;
	return std::nullopt;
}
