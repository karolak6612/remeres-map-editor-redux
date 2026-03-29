#include "app/visuals.h"

#include "util/file_system.h"

#include <fstream>

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

template <typename T>
T tableValueOr(const toml::table& table, std::string_view key, T fallback) {
	if (const auto value = table[key].template value<T>(); value.has_value()) {
		return *value;
	}
	return fallback;
}

}

bool Visuals::Load() {
	default_rules.clear();
	user_rules.clear();
	base_default_rules.clear();
	base_user_rules.clear();
	legacy_default_client_rules.clear();
	legacy_user_client_rules.clear();
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
	return LoadRulesFromFile(default_config_path, false, base_default_rules, legacy_default_client_rules);
}

bool Visuals::LoadUserOverrides() {
	return LoadRulesFromFile(user_config_path, true, base_user_rules, legacy_user_client_rules);
}

bool Visuals::LoadRulesFromFile(const wxString& path, bool user_file, RuleMap& destination, std::vector<VisualRule>& client_rules) {
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
	return SaveRulesToFile(user_config_path, user_rules);
}

bool Visuals::ExportUserOverrides(const wxString& path) const {
	EnsureServerItemRulesMaterialized();
	return SaveRulesToFile(path, user_rules);
}

bool Visuals::ImportUserOverrides(const wxString& path) {
	RuleMap imported_rules;
	std::vector<VisualRule> imported_client_rules;
	if (!LoadRulesFromFile(path, true, imported_rules, imported_client_rules)) {
		return false;
	}

	base_user_rules = std::move(imported_rules);
	legacy_user_client_rules = std::move(imported_client_rules);
	InvalidateResolvedRules();
	return true;
}

bool Visuals::SaveRulesToFile(const wxString& path, const RuleMap& rules) const {
	toml::table root;

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
