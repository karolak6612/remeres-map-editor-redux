#include "item_definitions/core/item_definition_resolver.h"
#include "item_definitions/formats/dat/dat_catalog.h"

#include <format>
#include <unordered_map>
#include <unordered_set>

namespace {
	constexpr uint64_t flagMask(ItemFlag flag) {
		return uint64_t { 1 } << static_cast<uint8_t>(flag);
	}

	// Check if a DAT item is empty/invalid and should be skipped
	bool isDatItemEmptyOrInvalid(const DatItemFragment& dat, const DatCatalog* catalog, ClientItemId client_id) {
		// Skip items with no attributes (all defaults)
		if (dat.group == ITEM_GROUP_NONE &&
			dat.type == ITEM_TYPE_NONE &&
			dat.flags == 0 &&
			dat.way_speed == 100 &&
			dat.always_on_top_order == 0) {
			return true;
		}

		// Skip items with no sprites, or only sprite ID 0 (empty placeholder)
		if (catalog) {
			const auto* entry = catalog->entry(client_id);
			if (entry) {
				if (entry->numsprites == 0) {
					return true;
				}
				// Check if all sprite IDs are 0 (empty placeholder sprites)
				bool allSpritesEmpty = true;
				for (uint32_t sprite_id : entry->sprite_ids) {
					if (sprite_id != 0) {
						allSpritesEmpty = false;
						break;
					}
				}
				if (allSpritesEmpty) {
					return true;
				}
			}
		}

		return false;
	}
}

bool ItemDefinitionResolver::resolve(const ItemDefinitionLoadInput& input, const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings, MissingItemReport* missingReport) {
	rows.clear();
	switch (input.mode) {
		case ItemDefinitionMode::DatOtb:
			return resolveDatOtb(input, fragments, rows, error, warnings, missingReport);
		case ItemDefinitionMode::DatOnly:
			return resolveDatOnly(input, fragments, rows, error, warnings, missingReport);
		case ItemDefinitionMode::DatSrv:
		case ItemDefinitionMode::Protobuf:
			error = "Selected item definition mode is not implemented yet.";
			return false;
	}
	error = "Unknown item definition mode.";
	return false;
}

bool ItemDefinitionResolver::resolveDatOtb(const ItemDefinitionLoadInput& input, const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings, MissingItemReport* missingReport) {
	rows.reserve(fragments.otb.size());

	// Track which DAT client IDs are referenced by OTB
	std::unordered_set<ClientItemId> dat_ids_referenced_by_otb;

	for (const auto& [server_id, otb] : fragments.otb) {
		ResolvedItemDefinitionRow row;
		row.server_id = server_id;
		row.group = otb.group;
		row.type = otb.type;
		row.flags = otb.flags;
		row.volume = otb.volume;
		row.max_text_len = otb.max_text_len;
		row.slot_position = otb.slot_position;
		row.weapon_type = otb.weapon_type;
		row.classification = otb.classification;
		row.border_base_ground_id = otb.border_base_ground_id;
		row.border_group = otb.border_group;
		row.weight = otb.weight;
		row.attack = otb.attack;
		row.defense = otb.defense;
		row.armor = otb.armor;
		row.charges = otb.charges;
		row.rotate_to = otb.rotate_to;
		row.way_speed = otb.way_speed;
		row.always_on_top_order = otb.always_on_top_order;
		row.border_alignment = otb.border_alignment;
		row.name = otb.name;
		row.description = otb.description;

		const auto xml_it = fragments.xml.find(server_id);
		const ClientItemId effective_client_id = (xml_it != fragments.xml.end() && xml_it->second.client_id.has_value()) ? *xml_it->second.client_id : otb.client_id;
		if (effective_client_id == 0) {
			warnings.push_back(std::format("Skipping items.otb entry {} because it does not define a DAT client id.", server_id));
			continue;
		}

		row.client_id = effective_client_id;

		const auto dat_it = fragments.dat.find(effective_client_id);
		if (dat_it == fragments.dat.end()) {
			// DAT item truly doesn't exist - collect as missing
			if (missingReport) {
				MissingItemEntry entry;
				entry.server_id = server_id;
				entry.client_id = effective_client_id;
				entry.name = otb.name;
				entry.description = otb.description;
				missingReport->missing_in_dat.push_back(std::move(entry));
			} else {
				error = wxString::FromUTF8(std::format("Missing DAT definition for client id {} (server id {}).", effective_client_id, server_id));
				return false;
			}
			// Continue processing other items
			continue;
		}

		// DAT item exists (even if empty) - this is a valid OTB mapping
		dat_ids_referenced_by_otb.insert(effective_client_id);
		row.flags |= dat_it->second.flags & ~flagMask(ItemFlag::Moveable);

		if (xml_it != fragments.xml.end()) {
			applyXmlOverrides(xml_it->second, row);
		}

		rows.push_back(std::move(row));
	}

	// Collect DAT items not referenced by OTB
	if (missingReport) {
		for (const auto& [client_id, dat] : fragments.dat) {
			// Skip empty/invalid DAT items
			if (isDatItemEmptyOrInvalid(dat, input.dat_catalog, client_id)) {
				continue;
			}
			if (dat_ids_referenced_by_otb.find(client_id) == dat_ids_referenced_by_otb.end()) {
				MissingItemEntry entry;
				entry.client_id = client_id;
				entry.server_id = 0; // Not mapped in OTB
				entry.name = "";
				missingReport->missing_in_otb.push_back(std::move(entry));
			}
		}

		// Collect XML entries that reference non-existent OTB server IDs
		for (const auto& [server_id, xml] : fragments.xml) {
			// Skip XML entries with invalid IDs
			if (server_id <= 0) {
				continue;
			}
			// Skip server-side fluid types and special items (IDs < 100)
			if (server_id < 100) {
				continue;
			}
			if (fragments.otb.find(server_id) == fragments.otb.end()) {
				MissingItemEntry entry;
				entry.server_id = server_id;
				entry.client_id = xml.client_id.value_or(0);
				entry.name = xml.name;
				entry.description = xml.description;
				missingReport->xml_no_otb.push_back(std::move(entry));
			}
		}

		// Collect OTB entries that don't have XML entries (informational)
		for (const auto& [server_id, otb] : fragments.otb) {
			// Skip OTB entries with no client ID
			if (otb.client_id == 0) {
				continue;
			}
			// Skip entries where DAT definition doesn't exist (already reported)
			if (fragments.dat.find(otb.client_id) == fragments.dat.end()) {
				continue;
			}
			// Skip entries that have XML overrides
			if (fragments.xml.find(server_id) != fragments.xml.end()) {
				continue;
			}
			MissingItemEntry entry;
			entry.server_id = server_id;
			entry.client_id = otb.client_id;
			entry.name = otb.name;
			entry.description = otb.description;
			missingReport->otb_no_xml.push_back(std::move(entry));
		}
	}

	if (rows.empty() && missingReport && missingReport->missing_in_dat.empty()) {
		error = "No item definitions were resolved from DAT/OTB/XML.";
		return false;
	}
	return true;
}

bool ItemDefinitionResolver::resolveDatOnly(const ItemDefinitionLoadInput& input, const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings, MissingItemReport* missingReport) {
	rows.reserve(fragments.dat.size());

	std::unordered_map<ClientItemId, size_t> client_to_row;
	client_to_row.reserve(fragments.dat.size());
	std::unordered_map<ClientItemId, ServerItemId> xml_server_by_client;
	xml_server_by_client.reserve(fragments.xml.size());

	// Track which DAT client IDs are used by XML
	std::unordered_set<ClientItemId> dat_ids_used_by_xml;

	for (const auto& [client_id, dat] : fragments.dat) {
		// Skip empty/invalid DAT items
		if (isDatItemEmptyOrInvalid(dat, input.dat_catalog, client_id)) {
			continue;
		}

		ResolvedItemDefinitionRow row;
		row.server_id = client_id;
		row.client_id = client_id;
		row.group = dat.group;
		row.type = dat.type;
		row.flags = dat.flags;
		row.way_speed = dat.way_speed;
		row.always_on_top_order = dat.always_on_top_order;
		rows.push_back(std::move(row));
		client_to_row[client_id] = rows.size() - 1;
	}

	for (const auto& [server_id, xml] : fragments.xml) {
		// Skip XML entries with invalid IDs
		if (server_id <= 0) {
			continue;
		}
		// Skip server-side fluid types and special items (IDs < 100)
		if (server_id < 100) {
			continue;
		}

		const ClientItemId client_id = xml.client_id.value_or(server_id);
		const auto row_it = client_to_row.find(client_id);
		if (row_it == client_to_row.end()) {
			// Check if DAT item exists but is empty/invalid
			const auto dat_it = fragments.dat.find(client_id);
			if (dat_it != fragments.dat.end() && isDatItemEmptyOrInvalid(dat_it->second, input.dat_catalog, client_id)) {
				// DAT item exists but is empty - skip XML entry silently
				continue;
			}

			// DAT item truly missing - collect missing instead of just warning
			if (missingReport) {
				MissingItemEntry entry;
				entry.server_id = server_id;
				entry.client_id = client_id;
				entry.name = xml.name;
				entry.description = xml.description;
				missingReport->missing_in_dat.push_back(std::move(entry));
			} else {
				warnings.push_back(std::format("Skipping items.xml entry {} in dat_only mode because DAT client id {} is missing.", server_id, client_id));
			}
			continue;
		}

		dat_ids_used_by_xml.insert(client_id);

		const auto owner_it = xml_server_by_client.find(client_id);
		if (owner_it != xml_server_by_client.end() && owner_it->second != server_id) {
			warnings.push_back(std::format("Duplicate items.xml client id {} in dat_only mode for server ids {} and {}. Keeping the first mapping.", client_id, owner_it->second, server_id));
			continue;
		}
		if (owner_it == xml_server_by_client.end()) {
			xml_server_by_client.emplace(client_id, server_id);
		}

		ResolvedItemDefinitionRow& row = rows[row_it->second];
		row.server_id = server_id;
		applyXmlOverrides(xml, row);
	}

	// Collect DAT items not referenced by XML (informational)
	if (missingReport && !fragments.xml.empty()) {
		for (const auto& [client_id, dat] : fragments.dat) {
			// Skip empty/invalid DAT items
			if (isDatItemEmptyOrInvalid(dat, input.dat_catalog, client_id)) {
				continue;
			}
			if (dat_ids_used_by_xml.find(client_id) == dat_ids_used_by_xml.end()) {
				MissingItemEntry entry;
				entry.client_id = client_id;
				entry.server_id = 0;
				entry.name = "";
				missingReport->missing_in_otb.push_back(std::move(entry));
			}
		}
	}

	if (rows.empty()) {
		error = "No item definitions were resolved from DAT/XML.";
		return false;
	}
	return true;
}

void ItemDefinitionResolver::applyXmlOverrides(const XmlItemFragment& xml, ResolvedItemDefinitionRow& row) {
	if (xml.client_id.has_value()) {
		row.client_id = *xml.client_id;
	}
	if (!xml.name.empty()) {
		row.name = xml.name;
	}
	if (!xml.editor_suffix.empty()) {
		row.editor_suffix = xml.editor_suffix;
	}
	if (!xml.description.empty()) {
		row.description = xml.description;
	}
	if (xml.group.has_value()) {
		row.group = *xml.group;
	}
	if (xml.type.has_value()) {
		row.type = *xml.type;
	}
	row.flags |= xml.flags;
	if (xml.volume.has_value()) {
		row.volume = *xml.volume;
	}
	if (xml.max_text_len.has_value()) {
		row.max_text_len = *xml.max_text_len;
	}
	if (xml.slot_position.has_value()) {
		row.slot_position = *xml.slot_position;
	}
	if (xml.weapon_type.has_value()) {
		row.weapon_type = *xml.weapon_type;
	}
	if (xml.weight.has_value()) {
		row.weight = *xml.weight;
	}
	if (xml.attack.has_value()) {
		row.attack = *xml.attack;
	}
	if (xml.defense.has_value()) {
		row.defense = *xml.defense;
	}
	if (xml.armor.has_value()) {
		row.armor = *xml.armor;
	}
	if (xml.charges.has_value()) {
		row.charges = *xml.charges;
	}
	if (xml.rotate_to.has_value()) {
		row.rotate_to = *xml.rotate_to;
	}
	if (xml.way_speed.has_value()) {
		row.way_speed = *xml.way_speed;
	}
}
