//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ITEM_DEFINITIONS_CORE_MISSING_ITEM_REPORT_H_
#define RME_ITEM_DEFINITIONS_CORE_MISSING_ITEM_REPORT_H_

#include <cstdint>
#include <string>
#include <vector>

struct MissingItemEntry {
	uint32_t server_id = 0;
	uint32_t client_id = 0;
	std::string name;
	std::string description;
};

struct MissingItemReport {
	std::vector<MissingItemEntry> missing_in_dat;   // In OTB/XML but not in DAT
	std::vector<MissingItemEntry> missing_in_otb;   // In DAT but not in OTB
	std::vector<MissingItemEntry> xml_no_otb;       // In XML but not in OTB
};

#endif
