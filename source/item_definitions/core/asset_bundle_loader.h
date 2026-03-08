#ifndef RME_ITEM_DEFINITIONS_CORE_ASSET_BUNDLE_LOADER_H_
#define RME_ITEM_DEFINITIONS_CORE_ASSET_BUNDLE_LOADER_H_

#include "item_definitions/core/asset_bundle.h"

#include <string>
#include <vector>

class GraphicManager;
class ItemDefinitionStore;
class wxString;

class AssetBundleLoader {
public:
	bool load(const AssetLoadRequest& request, AssetBundle& bundle, wxString& error, std::vector<std::string>& warnings) const;
	bool install(AssetBundle& bundle, GraphicManager& graphics, ItemDefinitionStore& store, wxString& error, std::vector<std::string>& warnings) const;
};

#endif
