#include "ui/find_item_window_model.h"

#include "app/settings.h"
#include "brushes/creature/creature_brush.h"
#include "brushes/raw/raw_brush.h"
#include "game/creatures.h"
#include "item_definitions/core/item_definition_store.h"
#include "rendering/core/game_sprite.h"
#include "ui/gui.h"
#include "util/common.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>
#include <tuple>

namespace {
	template <typename Enum>
	constexpr size_t enumCount() {
		return static_cast<size_t>(Enum::Count);
	}

	struct CatalogMatch {
		size_t index = 0;
		int score = 0;
	};

	[[nodiscard]] std::string trimCopy(std::string_view value) {
		const auto first = value.find_first_not_of(" \t\r\n");
		if (first == std::string_view::npos) {
			return {};
		}
		const auto last = value.find_last_not_of(" \t\r\n");
		return std::string(value.substr(first, last - first + 1));
	}

	[[nodiscard]] std::optional<uint16_t> parseItemId(std::string_view value) {
		const auto trimmed = trimCopy(value);
		if (trimmed.empty()) {
			return std::nullopt;
		}

		uint32_t parsed = 0;
		const auto* begin = trimmed.data();
		const auto* end = begin + trimmed.size();
		const auto [ptr, error] = std::from_chars(begin, end, parsed);
		if (error != std::errc {} || ptr != end || parsed > std::numeric_limits<uint16_t>::max()) {
			return std::nullopt;
		}

		return static_cast<uint16_t>(parsed);
	}

	[[nodiscard]] bool isSubsequence(std::string_view needle, std::string_view haystack) {
		if (needle.empty()) {
			return true;
		}

		size_t needle_index = 0;
		for (const char ch : haystack) {
			if (needle[needle_index] == ch) {
				++needle_index;
				if (needle_index == needle.size()) {
					return true;
				}
			}
		}
		return false;
	}

	[[nodiscard]] bool editDistanceAtMostOne(std::string_view lhs, std::string_view rhs) {
		if (lhs == rhs) {
			return true;
		}

		if (lhs.empty() || rhs.empty()) {
			return std::max(lhs.size(), rhs.size()) <= 1;
		}

		if (lhs.size() > rhs.size()) {
			std::swap(lhs, rhs);
		}

		if (rhs.size() - lhs.size() > 1) {
			return false;
		}

		size_t i = 0;
		size_t j = 0;
		bool used_edit = false;
		while (i < lhs.size() && j < rhs.size()) {
			if (lhs[i] == rhs[j]) {
				++i;
				++j;
				continue;
			}

			if (used_edit) {
				return false;
			}
			used_edit = true;

			if (lhs.size() == rhs.size()) {
				++i;
				++j;
			} else {
				++j;
			}
		}

		return true;
	}

	[[nodiscard]] std::vector<std::string> tokenizeLower(std::string_view value) {
		std::vector<std::string> tokens;
		std::string current;

		for (const unsigned char ch : value) {
			if (std::isalnum(ch) != 0) {
				current.push_back(static_cast<char>(ch));
				continue;
			}

			if (!current.empty()) {
				tokens.push_back(std::move(current));
				current.clear();
			}
		}

		if (!current.empty()) {
			tokens.push_back(std::move(current));
		}

		return tokens;
	}

	[[nodiscard]] int fuzzyMatchScore(const AdvancedFinderCatalogRow& row, std::string_view query_lower) {
		if (query_lower.empty()) {
			return 0;
		}

		if (row.lower_label == query_lower) {
			return 0;
		}

		for (const auto& token : row.name_tokens) {
			if (token == query_lower) {
				return 8;
			}
		}

		for (const auto& token : row.name_tokens) {
			if (token.starts_with(query_lower)) {
				return 16 + static_cast<int>(token.size() - query_lower.size());
			}
		}

		if (const auto position = row.lower_label.find(query_lower); position != std::string::npos) {
			return 32 + static_cast<int>(position);
		}

		if (isSubsequence(query_lower, row.lower_label)) {
			return 64 + static_cast<int>(row.lower_label.size() - query_lower.size());
		}

		for (const auto& token : row.name_tokens) {
			if (editDistanceAtMostOne(query_lower, token)) {
				return 96 + static_cast<int>(token.size());
			}
		}

		return -1;
	}

	[[nodiscard]] bool matchesMasks(const AdvancedFinderCatalogRow& row, const AdvancedFinderQuery& query) {
		if (query.type_mask != 0 && (row.type_mask & query.type_mask) == 0) {
			return false;
		}
		if ((row.property_mask & query.property_mask) != query.property_mask) {
			return false;
		}
		if ((row.interaction_mask & query.interaction_mask) != query.interaction_mask) {
			return false;
		}
		if ((row.visual_mask & query.visual_mask) != query.visual_mask) {
			return false;
		}
		return true;
	}

	[[nodiscard]] int matchScore(const AdvancedFinderCatalogRow& row, const AdvancedFinderQuery& query) {
		if (!matchesMasks(row, query)) {
			return -1;
		}

		switch (query.find_by) {
			case AdvancedFinderFindByMode::FuzzyName: {
				if (query.text.empty()) {
					return query.hasAnyFilter() ? 0 : -1;
				}
				return fuzzyMatchScore(row, as_lower_str(query.text));
			}
			case AdvancedFinderFindByMode::ServerId: {
				const auto parsed = parseItemId(query.text);
				if (!parsed || !row.isItem() || row.server_id != *parsed) {
					return -1;
				}
				return 0;
			}
			case AdvancedFinderFindByMode::ClientId: {
				const auto parsed = parseItemId(query.text);
				if (!parsed || !row.isItem() || row.client_id != *parsed) {
					return -1;
				}
				return 0;
			}
		}

		return -1;
	}

	[[nodiscard]] AdvancedFinderFilterMask buildTypeMask(const ItemDefinitionView& definition) {
		AdvancedFinderFilterMask mask = 0;
		const auto slot_position = static_cast<uint16_t>(definition.attribute(ItemAttributeKey::SlotPosition));
		const auto weapon_type = static_cast<uint8_t>(definition.attribute(ItemAttributeKey::WeaponType));

		if (definition.isDepot()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Depot);
		}
		if (definition.isMailbox()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Mailbox);
		}
		if (definition.isTrashHolder()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::TrashHolder);
		}
		if (definition.isContainer()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Container);
		}
		if (definition.isDoor()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Door);
		}
		if (definition.isMagicField()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::MagicField);
		}
		if (definition.isTeleport()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Teleport);
		}
		if (definition.isBed()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Bed);
		}
		if (definition.isKey()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Key);
		}
		if (definition.isPodium()) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Podium);
		}
		if (weapon_type != WEAPON_NONE && weapon_type != WEAPON_AMMO) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Weapon);
		}
		if (weapon_type == WEAPON_AMMO) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Ammo);
		}
		if ((slot_position & SLOTP_HEAD) != 0 || (slot_position & SLOTP_NECKLACE) != 0 || (slot_position & SLOTP_ARMOR) != 0 ||
			(slot_position & SLOTP_LEGS) != 0 || (slot_position & SLOTP_FEET) != 0 || (slot_position & SLOTP_RING) != 0 ||
			((slot_position & SLOTP_AMMO) != 0 && weapon_type != WEAPON_AMMO)) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Armor);
		}
		if (definition.group() == ITEM_GROUP_RUNE) {
			mask |= advancedFinderBit(AdvancedFinderTypeFilter::Rune);
		}

		return mask;
	}

	[[nodiscard]] AdvancedFinderFilterMask buildPropertyMask(const ItemDefinitionView& definition) {
		AdvancedFinderFilterMask mask = 0;
		if (definition.hasFlag(ItemFlag::Unpassable)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::Unpassable);
		}
		if (!definition.hasFlag(ItemFlag::Moveable)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::Unmovable);
		}
		if (definition.hasFlag(ItemFlag::BlockMissiles)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::BlockMissiles);
		}
		if (definition.hasFlag(ItemFlag::BlockPathfinder)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::BlockPathfinder);
		}
		if (definition.hasFlag(ItemFlag::HasElevation)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::HasElevation);
		}
		if (definition.isFloorChange()) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::FloorChange);
		}
		if (definition.hasFlag(ItemFlag::FullTile)) {
			mask |= advancedFinderBit(AdvancedFinderPropertyFilter::FullTile);
		}
		return mask;
	}

	[[nodiscard]] AdvancedFinderFilterMask buildInteractionMask(const ItemDefinitionView& definition) {
		AdvancedFinderFilterMask mask = 0;
		if (definition.hasFlag(ItemFlag::CanReadText)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Readable);
		}
		if (definition.hasFlag(ItemFlag::CanWriteText)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Writeable);
		}
		if (definition.hasFlag(ItemFlag::Pickupable)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Pickupable);
		}
		if (definition.hasFlag(ItemFlag::ForceUse) || definition.hasFlag(ItemFlag::MultiUse)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::ForceUse);
		}
		if (definition.hasFlag(ItemFlag::AllowDistRead)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::DistRead);
		}
		if (definition.hasFlag(ItemFlag::Rotatable)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Rotatable);
		}
		if (definition.hasFlag(ItemFlag::IsHangable)) {
			mask |= advancedFinderBit(AdvancedFinderInteractionFilter::Hangable);
		}
		return mask;
	}

	[[nodiscard]] AdvancedFinderFilterMask buildVisualMask(const ItemDefinitionView& definition) {
		AdvancedFinderFilterMask mask = 0;
		const auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(definition.clientId()));

		if (sprite && sprite->hasLight()) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::HasLight);
		}
		if (sprite && (sprite->animator != nullptr || sprite->frames > 1)) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::Animation);
		}
		if (definition.attribute(ItemAttributeKey::AlwaysOnTopOrder) > 0) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::AlwaysTop);
		}
		if (definition.hasFlag(ItemFlag::IgnoreLook)) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::IgnoreLook);
		}
		if (definition.attribute(ItemAttributeKey::Charges) > 0 || definition.hasFlag(ItemFlag::ClientChargeable) || definition.hasFlag(ItemFlag::ExtraChargeable)) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::HasCharges);
		}
		if (definition.hasFlag(ItemFlag::ClientChargeable)) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::ClientCharges);
		}
		if (definition.hasFlag(ItemFlag::Decays)) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::Decays);
		}
		if (definition.attribute(ItemAttributeKey::WaySpeed) != 100) {
			mask |= advancedFinderBit(AdvancedFinderVisualFilter::HasSpeed);
		}
		return mask;
	}
}

AdvancedFinderPersistedState LoadAdvancedFinderPersistedState() {
	AdvancedFinderPersistedState state;

	state.query.find_by = static_cast<AdvancedFinderFindByMode>(std::clamp(g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_FIND_BY), 0, 2));
	state.query.type_mask = static_cast<AdvancedFinderFilterMask>(std::max(0, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_TYPE_FILTERS)));
	state.query.property_mask = static_cast<AdvancedFinderFilterMask>(std::max(0, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_PROPERTY_FILTERS)));
	state.query.interaction_mask = static_cast<AdvancedFinderFilterMask>(std::max(0, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_INTERACTION_FILTERS)));
	state.query.visual_mask = static_cast<AdvancedFinderFilterMask>(std::max(0, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_VISUAL_FILTERS)));
	state.query.text = g_settings.getString(Config::ADVANCED_ITEM_FINDER_QUERY_TEXT);

	state.selection.kind = static_cast<AdvancedFinderCatalogKind>(std::clamp(g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_SELECTED_KIND), 0, 1));
	state.selection.server_id = static_cast<ServerItemId>(std::clamp<int>(g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_SELECTED_SERVER_ID), 0, std::numeric_limits<uint16_t>::max()));
	state.selection.creature_name = as_lower_str(g_settings.getString(Config::ADVANCED_ITEM_FINDER_SELECTED_CREATURE));
	state.last_action = static_cast<AdvancedFinderDefaultAction>(std::clamp(g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_LAST_ACTION), 0, 1));

	const auto position_x = g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_X);
	const auto position_y = g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_Y);
	if (position_x >= 0 && position_y >= 0) {
		state.position = wxPoint(position_x, position_y);
	}

	state.size = wxSize(
		std::max(900, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_WIDTH)),
		std::max(680, g_settings.getInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_HEIGHT))
	);

	return state;
}

void SaveAdvancedFinderPersistedState(const AdvancedFinderPersistedState& state) {
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_FIND_BY, static_cast<int>(state.query.find_by));
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_TYPE_FILTERS, static_cast<int>(state.query.type_mask));
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_PROPERTY_FILTERS, static_cast<int>(state.query.property_mask));
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_INTERACTION_FILTERS, static_cast<int>(state.query.interaction_mask));
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_VISUAL_FILTERS, static_cast<int>(state.query.visual_mask));
	g_settings.setString(Config::ADVANCED_ITEM_FINDER_QUERY_TEXT, state.query.text);
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_SELECTED_KIND, static_cast<int>(state.selection.kind));
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_SELECTED_SERVER_ID, state.selection.server_id);
	g_settings.setString(Config::ADVANCED_ITEM_FINDER_SELECTED_CREATURE, state.selection.creature_name);
	g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_LAST_ACTION, static_cast<int>(state.last_action));

	if (state.position != wxDefaultPosition && state.position.x >= 0 && state.position.y >= 0) {
		g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_X, state.position.x);
		g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_Y, state.position.y);
	}

	if (state.size.IsFullySpecified()) {
		g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_WIDTH, state.size.GetWidth());
		g_settings.setInteger(Config::ADVANCED_ITEM_FINDER_WINDOW_HEIGHT, state.size.GetHeight());
	}
}

void ApplyLegacySearchModeFallback(AdvancedFinderQuery& query, int legacy_search_mode) {
	switch (legacy_search_mode) {
		case 0:
			query.find_by = AdvancedFinderFindByMode::ServerId;
			break;
		case 1:
			query.find_by = AdvancedFinderFindByMode::ClientId;
			break;
		default:
			query.find_by = AdvancedFinderFindByMode::FuzzyName;
			break;
	}
}

int DeriveLegacySearchMode(const AdvancedFinderQuery& query) {
	if (query.find_by == AdvancedFinderFindByMode::ServerId) {
		return 0;
	}
	if (query.find_by == AdvancedFinderFindByMode::ClientId) {
		return 1;
	}
	if (query.property_mask != 0 || query.interaction_mask != 0 || query.visual_mask != 0) {
		return 4;
	}
	if (query.type_mask != 0) {
		return 3;
	}
	return 2;
}

std::vector<AdvancedFinderCatalogRow> BuildAdvancedFinderCatalog(bool include_creatures) {
	std::vector<AdvancedFinderCatalogRow> catalog;
	catalog.reserve(g_item_definitions.allIds().size() + (include_creatures ? 256 : 0));

	for (const ServerItemId id : g_item_definitions.allIds()) {
		const auto definition = g_item_definitions.get(id);
		if (!definition) {
			continue;
		}

		RAWBrush* raw_brush = definition.editorData().raw_brush;
		if (raw_brush == nullptr) {
			continue;
		}

		AdvancedFinderCatalogRow row;
		row.kind = AdvancedFinderCatalogKind::Item;
		row.brush = raw_brush;
		row.raw_brush = raw_brush;
		row.server_id = id;
		row.client_id = definition.clientId();
		row.label = std::string(definition.name());
		if (row.label.empty()) {
			row.label = raw_brush->getName();
		}
		row.lower_label = as_lower_str(row.label);
		row.name_tokens = tokenizeLower(row.lower_label);
		row.secondary_label = std::format("SID {}  CID {}", row.server_id, row.client_id);
		row.type_mask = buildTypeMask(definition);
		row.property_mask = buildPropertyMask(definition);
		row.interaction_mask = buildInteractionMask(definition);
		row.visual_mask = buildVisualMask(definition);
		catalog.push_back(std::move(row));
	}

	if (include_creatures) {
		for (auto it = g_creatures.begin(); it != g_creatures.end(); ++it) {
			CreatureType* creature_type = it->second;
			if (creature_type == nullptr || creature_type->brush == nullptr) {
				continue;
			}

			AdvancedFinderCatalogRow row;
			row.kind = AdvancedFinderCatalogKind::Creature;
			row.brush = creature_type->brush;
			row.creature_brush = creature_type->brush;
			row.label = creature_type->name;
			row.lower_label = as_lower_str(row.label);
			row.name_tokens = tokenizeLower(row.lower_label);
			row.secondary_label = "Creature";
			row.type_mask = advancedFinderBit(AdvancedFinderTypeFilter::Creature);
			catalog.push_back(std::move(row));
		}
	}

	return catalog;
}

std::vector<size_t> FilterAdvancedFinderCatalog(const std::vector<AdvancedFinderCatalogRow>& catalog, const AdvancedFinderQuery& query) {
	std::vector<CatalogMatch> matches;
	matches.reserve(catalog.size());

	for (size_t index = 0; index < catalog.size(); ++index) {
		const int score = matchScore(catalog[index], query);
		if (score >= 0) {
			matches.push_back(CatalogMatch { index, score });
		}
	}

	std::ranges::sort(matches, [&](const CatalogMatch& lhs, const CatalogMatch& rhs) {
		const auto& lhs_row = catalog[lhs.index];
		const auto& rhs_row = catalog[rhs.index];
		return std::tie(lhs.score, lhs_row.lower_label, lhs_row.server_id, lhs_row.client_id) <
			std::tie(rhs.score, rhs_row.lower_label, rhs_row.server_id, rhs_row.client_id);
	});

	std::vector<size_t> filtered_indices;
	filtered_indices.reserve(matches.size());
	for (const auto& match : matches) {
		filtered_indices.push_back(match.index);
	}
	return filtered_indices;
}

AdvancedFinderSelectionKey MakeAdvancedFinderSelectionKey(const AdvancedFinderCatalogRow& row) {
	AdvancedFinderSelectionKey key;
	key.kind = row.kind;
	key.server_id = row.server_id;
	if (row.isCreature()) {
		key.creature_name = row.lower_label;
	}
	return key;
}

bool AdvancedFinderSelectionMatches(const AdvancedFinderCatalogRow& row, const AdvancedFinderSelectionKey& selection) {
	if (row.kind != selection.kind) {
		return false;
	}

	if (row.isItem()) {
		return row.server_id != 0 && row.server_id == selection.server_id;
	}

	return row.lower_label == selection.creature_name;
}
