#include "item_definitions/core/item_definition_store_builder.h"

void ItemDefinitionStoreBuilder::build(ItemDefinitionStore& store, const ItemDefinitionVersionInfo& version, std::vector<ResolvedItemDefinitionRow>& rows) {
	store.clear();
	store.reserve(rows.size());
	for (auto& row : rows) {
		store.append(std::move(row));
	}
	store.MajorVersion = version.major_version;
	store.MinorVersion = version.minor_version;
	store.BuildNumber = version.build_number;
}
