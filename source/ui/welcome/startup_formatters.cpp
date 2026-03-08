#include "ui/welcome/startup_formatters.h"

#include "app/client_version.h"
#include "ui/theme.h"
#include "util/image_manager.h"

namespace {
wxString fallbackValue(const wxString& value) {
	return value.empty() ? wxString("-") : value;
}

wxString formatUnsigned(uint32_t value) {
	return value == 0 ? wxString("-") : wxString::Format("%u", value);
}

wxString formatMapDescription(const OTBMStartupPeekResult* info) {
	if (!info || info->description.empty()) {
		return "-";
	}
	return info->description;
}

wxString formatOtbmVersions(ClientVersion* client) {
	if (!client) {
		return "-";
	}

	wxString value;
	for (const auto map_version : client->getMapVersionsSupported()) {
		if (!value.empty()) {
			value += ", ";
		}
		value += wxString::Format("%d", static_cast<int>(map_version) + 1);
	}
	return value.empty() ? wxString("-") : value;
}
}

wxColour StartupStatusColour(StartupCompatibilityStatus status) {
	switch (status) {
		case StartupCompatibilityStatus::Compatible:
			return Theme::Get(Theme::Role::Success);
		case StartupCompatibilityStatus::Forced:
		case StartupCompatibilityStatus::ForceRequired:
		case StartupCompatibilityStatus::MapError:
			return Theme::Get(Theme::Role::Warning);
		case StartupCompatibilityStatus::MissingSelection:
		default:
			return Theme::Get(Theme::Role::TextSubtle);
	}
}

std::vector<StartupInfoField> BuildStartupMapFields(const OTBMStartupPeekResult* info) {
	if (!info || info->has_error) {
		return {
			{ "Map Name", "-", std::string(ICON_FILE) },
			{ "Client Version", "Placeholder", std::string(ICON_CIRCLE_INFO) },
			{ "OTBM Version", "-", std::string(ICON_LIST) },
			{ "Items Major Version", "-", std::string(ICON_LIST) },
			{ "Items Minor Version", "-", std::string(ICON_LIST) },
			{ "House XML File", "-", std::string(ICON_FILE) },
			{ "Spawn XML File", "-", std::string(ICON_FILE) },
			{ "Description", "-", std::string(ICON_FILE_LINES) },
		};
	}

	return {
		{ "Map Name", fallbackValue(info->map_name), std::string(ICON_FILE) },
		{ "Client Version", fallbackValue(info->client_version), std::string(ICON_CIRCLE_INFO) },
		{ "OTBM Version", info->otbm_version > 0 ? wxString::Format("%d", info->otbm_version) : wxString("-"), std::string(ICON_LIST) },
		{ "Items Major Version", formatUnsigned(info->items_major_version), std::string(ICON_LIST) },
		{ "Items Minor Version", formatUnsigned(info->items_minor_version), std::string(ICON_LIST) },
		{ "House XML File", fallbackValue(info->house_xml_file), std::string(ICON_FILE) },
		{ "Spawn XML File", fallbackValue(info->spawn_xml_file), std::string(ICON_FILE) },
		{ "Description", formatMapDescription(info), std::string(ICON_FILE_LINES) },
	};
}

std::vector<StartupInfoField> BuildStartupClientFields(ClientVersion* client) {
	if (!client) {
		return {
			{ "Client Name", "-", std::string(ICON_HARD_DRIVE) },
			{ "Client Version", "-", std::string(ICON_CIRCLE_INFO) },
			{ "Data Directory", "-", std::string(ICON_FOLDER) },
			{ "OTBM Version(s)", "-", std::string(ICON_LIST) },
			{ "Items Major Version", "-", std::string(ICON_LIST) },
			{ "Items Minor Version", "-", std::string(ICON_LIST) },
			{ "DAT Signature", "-", std::string(ICON_FILE) },
			{ "SPR Signature", "-", std::string(ICON_FILE) },
			{ "Description", "-", std::string(ICON_FILE_LINES) },
		};
	}

	return {
		{ "Client Name", wxstr(client->getName()), std::string(ICON_HARD_DRIVE) },
		{ "Client Version", wxString::Format("%u", client->getVersion()), std::string(ICON_CIRCLE_INFO) },
		{ "Data Directory", wxstr(client->getDataDirectory()), std::string(ICON_FOLDER) },
		{ "OTBM Version(s)", formatOtbmVersions(client), std::string(ICON_LIST) },
		{ "Items Major Version", wxString::Format("%u", client->getOtbMajor()), std::string(ICON_LIST) },
		{ "Items Minor Version", wxString::Format("%u", client->getOtbId()), std::string(ICON_LIST) },
		{ "DAT Signature", wxString::Format("%X", client->getDatSignature()), std::string(ICON_FILE) },
		{ "SPR Signature", wxString::Format("%X", client->getSprSignature()), std::string(ICON_FILE) },
		{ "Description", wxstr(client->getDescription()), std::string(ICON_FILE_LINES) },
	};
}
