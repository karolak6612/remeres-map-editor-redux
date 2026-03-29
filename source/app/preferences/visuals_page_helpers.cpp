#include "app/preferences/visuals_page_helpers.h"

#include <algorithm>
#include <cctype>
#include <ranges>

#include <wx/button.h>
#include <wx/filename.h>

namespace {

bool IsAbsolutePath(const std::string& path) {
	return !path.empty() && wxFileName(wxString::FromUTF8(path)).IsAbsolute();
}

wxString ValidationLabelFor(const VisualRule& rule) {
	if (!rule.valid) {
		return wxString::FromUTF8(rule.validation_error);
	}

	switch (rule.appearance.type) {
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (rule.appearance.asset_path.empty()) {
				return "Missing image path.";
			}
			if (!IsAbsolutePath(rule.appearance.asset_path)) {
				return "Image path must be absolute.";
			}
			if (!wxFileName(wxString::FromUTF8(rule.appearance.asset_path)).FileExists()) {
				return "Image file is missing.";
			}
			break;
		default:
			break;
	}

	return "Valid";
}

}

namespace VisualsPageHelpers {

std::string LowercaseCopy(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

std::string MatchLabel(const VisualRule& rule) {
	switch (rule.match_type) {
		case VisualMatchType::ItemId:
			return "Item ID " + std::to_string(rule.match_id);
		case VisualMatchType::ClientId:
			return "Legacy compatibility entry " + std::to_string(rule.match_id);
		case VisualMatchType::Marker:
			return "Marker: " + rule.match_value;
		case VisualMatchType::Overlay:
			return "Overlay: " + rule.match_value;
		case VisualMatchType::Tile:
			return "Tile: " + rule.match_value;
	}
	return "Visual";
}

wxString CatalogLabel(const VisualCatalogEntry& entry) {
	const VisualRule* rule = entry.effective();
	if (!rule) {
		return "Visual";
	}

	wxString label = wxString::FromUTF8(rule->label);
	if (entry.hasOverride()) {
		label += " [Override]";
	}
	if (!rule->valid) {
		label += " [Invalid]";
	}
	return label;
}

wxString CurrentValueLabel(const VisualRule& rule) {
	if (!rule.valid) {
		return ValidationLabelFor(rule);
	}

	switch (rule.appearance.type) {
		case VisualAppearanceType::OtherItemVisual:
			return wxString::Format("Sprite: item %u", static_cast<unsigned>(rule.appearance.item_id));
		case VisualAppearanceType::SpriteId:
			return wxString::Format("Sprite: built-in %u", rule.appearance.sprite_id);
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg: {
			wxString label = wxString::FromUTF8(rule.appearance.asset_path);
			if (label.empty()) {
				label = "SVG / PNG not selected";
			}
			if (!IsNeutralColor(rule.appearance.color)) {
				label += " + tint";
			}
			return label;
		}
		case VisualAppearanceType::Rgba:
			return wxString::Format(
				"Color: rgba(%u, %u, %u, %u)",
				rule.appearance.color.Red(),
				rule.appearance.color.Green(),
				rule.appearance.color.Blue(),
				rule.appearance.color.Alpha()
			);
	}
	return {};
}

bool IsNeutralColor(const wxColour& color) {
	return color.IsOk() && color.Red() == 255 && color.Green() == 255 && color.Blue() == 255 && color.Alpha() == 255;
}

void StyleModeButton(wxButton* button, bool active) {
	if (!button) {
		return;
	}

	if (active) {
		button->SetBackgroundColour(wxColour(74, 106, 160));
		button->SetForegroundColour(*wxWHITE);
	} else {
		button->SetBackgroundColour(wxNullColour);
		button->SetForegroundColour(wxNullColour);
	}
	button->Refresh();
}

TreeItemData::TreeItemData(Kind item_kind, std::string item_key, std::string item_group) :
	kind(item_kind),
	key(std::move(item_key)),
	group(std::move(item_group)) {
}

}
