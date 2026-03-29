#include "app/visuals.h"

#include "app/definitions.h"
#include "item_definitions/core/item_definition_store.h"

#include <ranges>

Visuals g_visuals;

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

void Visuals::ExpandClientRules(const std::vector<VisualRule>& source, std::map<std::string, VisualRule>& destination) {
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

std::string Visuals::GetApplicationName() {
	return __RME_APPLICATION_NAME__;
}

std::string Visuals::GetSiteUrl() {
	return __SITE_URL__;
}

std::string Visuals::GetAssetsName() {
	return ASSETS_NAME;
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
