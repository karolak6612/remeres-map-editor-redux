#include "item_definitions/core/item_definition_recipe.h"

const ItemDefinitionRecipe& ItemDefinitionRecipeRegistry::get(ItemDefinitionMode mode) {
	static const ItemDefinitionRecipe dat_otb {
		.mode = ItemDefinitionMode::DatOtb,
		.sources = { ItemDefinitionSourceKind::Dat, ItemDefinitionSourceKind::Otb, ItemDefinitionSourceKind::Xml },
		.source_count = 3,
		.runnable = true,
	};
	static const ItemDefinitionRecipe dat_only {
		.mode = ItemDefinitionMode::DatOnly,
		.sources = { ItemDefinitionSourceKind::Dat, ItemDefinitionSourceKind::Xml, ItemDefinitionSourceKind::Xml },
		.source_count = 2,
		.runnable = true,
	};
	static const ItemDefinitionRecipe dat_srv {
		.mode = ItemDefinitionMode::DatSrv,
		.sources = { ItemDefinitionSourceKind::Dat, ItemDefinitionSourceKind::Srv, ItemDefinitionSourceKind::Xml },
		.source_count = 3,
		.runnable = false,
	};
	static const ItemDefinitionRecipe protobuf {
		.mode = ItemDefinitionMode::Protobuf,
		.sources = { ItemDefinitionSourceKind::Protobuf, ItemDefinitionSourceKind::Xml, ItemDefinitionSourceKind::Xml },
		.source_count = 2,
		.runnable = true,
	};

	switch (mode) {
		case ItemDefinitionMode::DatOtb:
			return dat_otb;
		case ItemDefinitionMode::DatOnly:
			return dat_only;
		case ItemDefinitionMode::DatSrv:
			return dat_srv;
		case ItemDefinitionMode::Protobuf:
			return protobuf;
		default:
			return dat_otb;
	}
}
