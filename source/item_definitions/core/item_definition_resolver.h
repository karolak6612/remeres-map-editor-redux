#ifndef RME_ITEM_DEFINITION_RESOLVER_H_
#define RME_ITEM_DEFINITION_RESOLVER_H_

#include "item_definitions/core/item_definition_fragments.h"

class ItemDefinitionResolver {
public:
	static bool resolve(const ItemDefinitionLoadInput& input, const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings);

private:
	static bool resolveDatOtb(const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings);
	static bool resolveDatOnly(const ItemDefinitionFragments& fragments, std::vector<ResolvedItemDefinitionRow>& rows, wxString& error, std::vector<std::string>& warnings);
	static void applyXmlOverrides(const XmlItemFragment& xml, ResolvedItemDefinitionRow& row);
};

#endif
