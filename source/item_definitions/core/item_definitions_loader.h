#ifndef RME_ITEM_DEFINITIONS_LOADER_H_
#define RME_ITEM_DEFINITIONS_LOADER_H_

#include "item_definitions/core/item_definition_store.h"

class ItemDefinitionsLoader {
public:
	bool load(const ItemDefinitionLoadInput& input, wxString& error, std::vector<std::string>& warnings) const;
};

#endif
