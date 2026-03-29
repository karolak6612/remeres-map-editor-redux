#include "app/visuals.h"

#include "app/definitions.h"
#include "item_definitions/core/item_definition_store.h"
#include "util/file_system.h"

#include <fstream>
#include <ranges>

#include <spdlog/spdlog.h>
#include <toml++/toml.h>
#include <wx/filename.h>

namespace {
using RuleMap = std::map<std::string, VisualRule>;

std::string normalize(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

wxColour readColor(const toml::table& rule, std::string_view field_name, const wxColour& fallback) {
	const auto* color = rule[field_name].as_array();
	if (!color || color->size() < 4) {
		return fallback;
	}

	return wxColour(
		static_cast<unsigned char>((*color)[0].value_or(255)),
		static_cast<unsigned char>((*color)[1].value_or(255)),
		static_cast<unsigned char>((*color)[2].value_or(255)),
		static_cast<unsigned char>((*color)[3].value_or(255))
	);
}

toml::array writeColor(const wxColour& color) {
	return toml::array {
		color.Red(),
		color.Green(),
		color.Blue(),
		color.Alpha()
	};
}

wxString resolveDefaultsPath() {
	wxFileName file(FileSystem::GetDataDirectory(), "visuals.defaults.toml");
	if (file.FileExists()) {
		return file.GetFullPath();
	}

	file.Assign(FileSystem::GetExecDirectory());
	file.SetFullName("visuals.defaults.toml");
	if (file.FileExists()) {
		return file.GetFullPath();
	}

	file.Assign(FileSystem::GetFoundDataDirectory());
	file.SetFullName("visuals.defaults.toml");
	return file.GetFullPath();
}

std::string pathExtension(const std::string& path) {
	return normalize(wxFileName::FileName(path).GetExt().ToStdString());
}

template <typename T>
T tableValueOr(const toml::table& table, std::string_view key, T fallback) {
	if (const auto value = table[key].template value<T>(); value.has_value()) {
		return *value;
	}
	return fallback;
}
}

Visuals g_visuals;

bool Visuals::Load() {
	default_rules.clear();
	user_rules.clear();
	base_default_rules.clear();
	base_user_rules.clear();
	legacy_default_client_rules.clear();
	legacy_user_client_rules.clear();
	default_assets_name = ASSETS_NAME;
	user_assets_name.reset();
	resolved_signature = 0;
	resolved_rules_dirty = true;

	default_config_path = resolveDefaultsPath();
	user_config_path = FileSystem::GetLocalDirectory() + "visuals.toml";

	const bool defaults_loaded = LoadDefaults();
	const bool user_loaded = LoadUserOverrides();
	return defaults_loaded && user_loaded;
}

bool Visuals::LoadDefaults() {
	if (default_config_path.empty()) {
		return false;
	}
	std::optional<std::string> unused_override;
	return LoadRulesFromFile(default_config_path, false, base_default_rules, legacy_default_client_rules, unused_override);
}

bool Visuals::LoadUserOverrides() {
	std::optional<std::string> assets_override;
	const bool loaded = LoadRulesFromFile(user_config_path, true, base_user_rules, legacy_user_client_rules, assets_override);
	user_assets_name = std::move(assets_override);
	return loaded;
}

bool Visuals::LoadRulesFromFile(const wxString& path, bool user_file, RuleMap& destination, std::vector<VisualRule>& client_rules, std::optional<std::string>& assets_name_override) {
	if (path.empty() || !wxFileName::FileExists(path)) {
		return !user_file;
	}

	toml::table config;
	try {
		config = toml::parse_file(path.ToStdString());
	} catch (const toml::parse_error& error) {
		spdlog::error("Failed to parse {}: {}", path.ToStdString(), error.description());
		return false;
	}

	if (user_file) {
		if (const auto value = config["assets_name"].value<std::string>(); value.has_value()) {
			assets_name_override = value;
		}
	} else if (const auto value = config["assets_name"].value<std::string>(); value.has_value()) {
		default_assets_name = *value;
	}

	if (const auto* rules = config["rules"].as_array()) {
		for (const auto& node : *rules) {
			const auto* table = node.as_table();
			if (!table) {
				continue;
			}

			VisualRule rule;
			rule.key = tableValueOr<std::string>(*table, "key", {});
			rule.label = tableValueOr<std::string>(*table, "label", {});
			rule.group = tableValueOr<std::string>(*table, "group", {});
			rule.enabled = tableValueOr<bool>(*table, "enabled", true);

			if (const auto type = ParseMatchType(tableValueOr<std::string>(*table, "subject", {})); type.has_value()) {
				rule.match_type = *type;
			}

			if (rule.match_type == VisualMatchType::ItemId || rule.match_type == VisualMatchType::ClientId) {
				rule.match_id = tableValueOr<int32_t>(*table, "value", 0);
			} else {
				rule.match_value = tableValueOr<std::string>(*table, "value", {});
			}

			if (const auto type = ParseAppearanceType(tableValueOr<std::string>(*table, "appearance", std::string("rgba"))); type.has_value()) {
				rule.appearance.type = *type;
			}

			rule.appearance.color = readColor(*table, "color", wxColour(255, 255, 255, 255));
			rule.appearance.sprite_id = tableValueOr<uint32_t>(*table, "sprite_id", 0u);
			rule.appearance.item_id = static_cast<uint16_t>(tableValueOr<int>(*table, "item_id", 0));
			rule.appearance.asset_path = tableValueOr<std::string>(*table, "path", {});

			if (rule.match_type == VisualMatchType::ClientId) {
				if (rule.key.empty()) {
					rule.key = "legacy.client." + std::to_string(rule.match_id);
				}
				rule.label = rule.label.empty() ? DeriveLabel(rule) : rule.label;
				rule.group = rule.group.empty() ? DeriveGroup(rule) : rule.group;
				ValidateRule(rule);
				client_rules.push_back(std::move(rule));
				continue;
			}

			rule.key = rule.key.empty() ? BuildKey(rule) : rule.key;
			rule.label = rule.label.empty() ? DeriveLabel(rule) : rule.label;
			rule.group = rule.group.empty() ? DeriveGroup(rule) : rule.group;
			ValidateRule(rule);
			destination[rule.key] = std::move(rule);
		}
	}

	return true;
}

bool Visuals::SaveUserOverrides() const {
	EnsureServerItemRulesMaterialized();
	return SaveRulesToFile(user_config_path, user_rules, user_assets_name);
}

bool Visuals::ExportUserOverrides(const wxString& path) const {
	EnsureServerItemRulesMaterialized();
	return SaveRulesToFile(path, user_rules, user_assets_name);
}

bool Visuals::ImportUserOverrides(const wxString& path) {
	RuleMap imported_rules;
	std::vector<VisualRule> imported_client_rules;
	std::optional<std::string> imported_assets_name;
	if (!LoadRulesFromFile(path, true, imported_rules, imported_client_rules, imported_assets_name)) {
		return false;
	}

	base_user_rules = std::move(imported_rules);
	legacy_user_client_rules = std::move(imported_client_rules);
	user_assets_name = std::move(imported_assets_name);
	InvalidateResolvedRules();
	return true;
}

void Visuals::EnsureServerItemRulesMaterialized() const {
	const uint64_t current_signature = CurrentItemDefinitionSignature();
	if (!resolved_rules_dirty && resolved_signature == current_signature) {
		return;
	}

	if (current_signature != 0 && !legacy_user_client_rules.empty()) {
		ExpandClientRules(legacy_user_client_rules, base_user_rules);
		legacy_user_client_rules.clear();
	}

	default_rules = base_default_rules;
	user_rules = base_user_rules;

	if (current_signature != 0) {
		ExpandClientRules(legacy_default_client_rules, default_rules);
		ExpandClientRules(legacy_user_client_rules, user_rules);
	}

	resolved_signature = current_signature;
	resolved_rules_dirty = false;
}

void Visuals::InvalidateResolvedRules() {
	resolved_rules_dirty = true;
	resolved_signature = 0;
}

uint64_t Visuals::CurrentItemDefinitionSignature() {
	if (g_item_definitions.allIds().empty()) {
		return 0;
	}

	uint64_t signature = static_cast<uint64_t>(g_item_definitions.MajorVersion) << 48;
	signature ^= static_cast<uint64_t>(g_item_definitions.MinorVersion) << 32;
	signature ^= static_cast<uint64_t>(g_item_definitions.BuildNumber) << 16;
	signature ^= static_cast<uint64_t>(g_item_definitions.maxServerId());
	signature ^= static_cast<uint64_t>(g_item_definitions.allIds().size());
	return signature;
}

void Visuals::ExpandClientRules(const std::vector<VisualRule>& source, RuleMap& destination) {
	for (const VisualRule& client_rule : source) {
		const auto server_ids = g_item_definitions.findAllByClientId(static_cast<ClientItemId>(client_rule.match_id));
		if (server_ids.empty()) {
			continue;
		}

		for (const ServerItemId server_id : server_ids) {
			VisualRule expanded_rule = client_rule;
			expanded_rule.match_type = VisualMatchType::ItemId;
			expanded_rule.match_id = server_id;
			expanded_rule.match_value.clear();
			expanded_rule.key = MakeKeyForItemId(server_id);
			expanded_rule.label = client_rule.label + " [Item ID " + std::to_string(server_id) + "]";
			expanded_rule.group = "Items";
			ValidateRule(expanded_rule);
			if (!destination.contains(expanded_rule.key)) {
				destination[expanded_rule.key] = std::move(expanded_rule);
			}
		}
	}
}

bool Visuals::SaveRulesToFile(const wxString& path, const RuleMap& rules, const std::optional<std::string>& assets_name_override) const {
	toml::table root;
	if (assets_name_override.has_value()) {
		root.insert_or_assign("assets_name", *assets_name_override);
	}

	toml::array serialized_rules;
	for (const auto& [key, rule] : rules) {
		toml::table table;
		table.insert_or_assign("key", key);
		table.insert_or_assign("label", rule.label);
		table.insert_or_assign("group", rule.group);
		table.insert_or_assign("subject", ToString(rule.match_type));
		if (rule.match_type == VisualMatchType::ItemId || rule.match_type == VisualMatchType::ClientId) {
			table.insert_or_assign("value", rule.match_id);
		} else {
			table.insert_or_assign("value", rule.match_value);
		}
		table.insert_or_assign("enabled", rule.enabled);
		table.insert_or_assign("appearance", ToString(rule.appearance.type));
		table.insert_or_assign("color", writeColor(rule.appearance.color));
		if (rule.appearance.sprite_id != 0) {
			table.insert_or_assign("sprite_id", rule.appearance.sprite_id);
		}
		if (rule.appearance.item_id != 0) {
			table.insert_or_assign("item_id", static_cast<int>(rule.appearance.item_id));
		}
		if (!rule.appearance.asset_path.empty()) {
			table.insert_or_assign("path", rule.appearance.asset_path);
		}
		serialized_rules.push_back(std::move(table));
	}
	root.insert_or_assign("rules", std::move(serialized_rules));

	std::ofstream file(path.ToStdString(), std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		spdlog::error("Failed to open {} for writing", path.ToStdString());
		return false;
	}

	file << root;
	return file.good();
}

std::vector<VisualCatalogEntry> Visuals::BuildCatalog() const {
	EnsureServerItemRulesMaterialized();
	std::vector<VisualCatalogEntry> catalog;
	catalog.reserve(default_rules.size() + user_rules.size());

	for (const auto& [key, rule] : default_rules) {
		VisualCatalogEntry entry;
		entry.default_rule = rule;
		if (const auto user_it = user_rules.find(key); user_it != user_rules.end()) {
			entry.user_rule = user_it->second;
		}
		catalog.push_back(std::move(entry));
	}

	for (const auto& [key, rule] : user_rules) {
		if (default_rules.contains(key)) {
			continue;
		}

		VisualCatalogEntry entry;
		entry.user_rule = rule;
		catalog.push_back(std::move(entry));
	}

	std::ranges::sort(catalog, [](const VisualCatalogEntry& lhs, const VisualCatalogEntry& rhs) {
		const auto* left = lhs.effective();
		const auto* right = rhs.effective();
		if (!left || !right) {
			return left != nullptr;
		}
		if (left->group != right->group) {
			return left->group < right->group;
		}
		return left->label < right->label;
	});

	return catalog;
}

const VisualRule* Visuals::GetDefaultRule(const std::string& key) const {
	EnsureServerItemRulesMaterialized();
	if (const auto it = default_rules.find(key); it != default_rules.end()) {
		return &it->second;
	}
	return nullptr;
}

const VisualRule* Visuals::GetUserRule(const std::string& key) const {
	EnsureServerItemRulesMaterialized();
	if (const auto it = user_rules.find(key); it != user_rules.end()) {
		return &it->second;
	}
	return nullptr;
}

const VisualRule* Visuals::GetEffectiveRule(const std::string& key) const {
	if (const auto* user_rule = GetUserRule(key)) {
		return user_rule;
	}
	return GetDefaultRule(key);
}

void Visuals::SetUserRule(VisualRule rule) {
	rule.key = rule.key.empty() ? BuildKey(rule) : rule.key;
	rule.label = rule.label.empty() ? DeriveLabel(rule) : rule.label;
	rule.group = rule.group.empty() ? DeriveGroup(rule) : rule.group;
	ValidateRule(rule);
	base_user_rules[rule.key] = std::move(rule);
	InvalidateResolvedRules();
}

void Visuals::RemoveUserRule(const std::string& key) {
	base_user_rules.erase(key);
	InvalidateResolvedRules();
}

void Visuals::ClearUserOverrides() {
	base_user_rules.clear();
	legacy_user_client_rules.clear();
	user_assets_name.reset();
	InvalidateResolvedRules();
}

const VisualRule* Visuals::ResolveItem(uint16_t item_id) const {
	EnsureServerItemRulesMaterialized();
	if (const auto* item_rule = GetEffectiveRule(MakeKeyForItemId(item_id)); item_rule && item_rule->enabled && item_rule->valid) {
		return item_rule;
	}
	return nullptr;
}

const VisualRule* Visuals::ResolveMarker(MarkerVisualKind kind) const {
	EnsureServerItemRulesMaterialized();
	if (const auto* rule = GetEffectiveRule(MakeKeyForMarker(kind)); rule && rule->enabled && rule->valid) {
		return rule;
	}
	return nullptr;
}

const VisualRule* Visuals::ResolveOverlay(OverlayVisualKind kind) const {
	EnsureServerItemRulesMaterialized();
	if (const auto* rule = GetEffectiveRule(MakeKeyForOverlay(kind)); rule && rule->enabled && rule->valid) {
		return rule;
	}
	return nullptr;
}

const VisualRule* Visuals::ResolveTile(TileVisualKind kind) const {
	EnsureServerItemRulesMaterialized();
	if (const auto* rule = GetEffectiveRule(MakeKeyForTile(kind)); rule && rule->enabled && rule->valid) {
		return rule;
	}
	return nullptr;
}

std::string Visuals::GetAssetsName() const {
	if (user_assets_name.has_value() && !user_assets_name->empty()) {
		return *user_assets_name;
	}
	return default_assets_name.empty() ? ASSETS_NAME : default_assets_name;
}

void Visuals::SetAssetsNameOverride(std::string assets_name) {
	if (assets_name.empty() || assets_name == default_assets_name) {
		user_assets_name.reset();
		return;
	}
	user_assets_name = std::move(assets_name);
}

std::string Visuals::MakeKeyForItemId(uint16_t item_id) {
	return "item.id." + std::to_string(item_id);
}

std::string Visuals::MakeKeyForMarker(MarkerVisualKind kind) {
	return "marker." + ToString(kind);
}

std::string Visuals::MakeKeyForOverlay(OverlayVisualKind kind) {
	return "overlay." + ToString(kind);
}

std::string Visuals::MakeKeyForTile(TileVisualKind kind) {
	return "tile." + ToString(kind);
}

VisualRule Visuals::MakeItemRule(uint16_t item_id) {
	VisualRule rule;
	rule.match_type = VisualMatchType::ItemId;
	rule.match_id = item_id;
	rule.key = MakeKeyForItemId(item_id);
	rule.label = DeriveLabel(rule);
	rule.group = DeriveGroup(rule);
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

bool Visuals::SupportsSpriteModes(const VisualRule& rule) {
	return rule.match_type == VisualMatchType::ItemId || rule.match_type == VisualMatchType::ClientId || rule.match_type == VisualMatchType::Marker || rule.match_type == VisualMatchType::Tile;
}

bool Visuals::SupportsImageModes(const VisualRule& WXUNUSED(rule)) {
	return true;
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
			return "Items";
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
