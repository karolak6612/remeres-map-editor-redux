#include "item_definitions/core/item_definition_resolver.h"

#include <format>
#include <unordered_map>

namespace {
	constexpr uint64_t flagMask(ItemFlag flag) {
		return uint64_t { 1 } << static_cast<uint8_t>(flag);
	}
}

bool ItemDefinitionResolver::resolve(const ItemDefinitionLoadInput& input, const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings) {
	rows.clear();
	switch (input.mode) {
		case ItemDefinitionMode::DatOtb:
			return resolveDatOtb(fragments, rows, error, warnings);
		case ItemDefinitionMode::DatOnly:
			return resolveDatOnly(fragments, rows, error, warnings);
		case ItemDefinitionMode::DatSrv:
		case ItemDefinitionMode::Protobuf:
			error = "Selected item definition mode is not implemented yet.";
			return false;
	}
	error = "Unknown item definition mode.";
	return false;
}

bool ItemDefinitionResolver::resolveDatOtb(const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings) {
	rows.reserve(fragments.otb.size());

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
			error = wxString::FromUTF8(std::format("Missing DAT definition for client id {} (server id {}).", effective_client_id, server_id));
			return false;
		}
		row.flags |= dat_it->second.flags & ~flagMask(ItemFlag::Moveable);

		if (xml_it != fragments.xml.end()) {
			applyXmlOverrides(xml_it->second, row);
		}

		rows.push_back(std::move(row));
	}

	if (rows.empty()) {
		error = "No item definitions were resolved from DAT/OTB/XML.";
		return false;
	}
	return true;
}

bool ItemDefinitionResolver::resolveDatOnly(const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings) {
	rows.reserve(fragments.dat.size());

	std::unordered_map<ClientItemId, size_t> client_to_row;
	client_to_row.reserve(fragments.dat.size());
	std::unordered_map<ClientItemId, ServerItemId> xml_server_by_client;
	xml_server_by_client.reserve(fragments.xml.size());

	for (const auto& [client_id, dat] : fragments.dat) {
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
		const ClientItemId client_id = xml.client_id.value_or(server_id);
		const auto row_it = client_to_row.find(client_id);
		if (row_it == client_to_row.end()) {
			warnings.push_back(std::format("Skipping items.xml entry {} in dat_only mode because DAT client id {} is missing.", server_id, client_id));
			continue;
		}
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
