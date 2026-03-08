#ifndef RME_ITEM_DEFINITION_STORE_BUILDER_H_
#define RME_ITEM_DEFINITION_STORE_BUILDER_H_

#include "item_definitions/core/item_definition_store.h"

class ItemDefinitionStoreBuilder {
public:
	static void build(ItemDefinitionStore& store, const ItemDefinitionVersionInfo& version, std::vector<ResolvedItemDefinitionRow>& rows);
};

#endif
