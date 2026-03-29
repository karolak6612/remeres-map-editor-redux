#ifndef RME_PREFERENCES_VISUALS_PAGE_HELPERS_H_
#define RME_PREFERENCES_VISUALS_PAGE_HELPERS_H_

#include "app/visuals.h"

#include <string>

#include <wx/string.h>
#include <wx/treectrl.h>

class wxButton;

namespace VisualsPageHelpers {

std::string LowercaseCopy(std::string value);
std::string MatchLabel(const VisualRule& rule);
wxString CatalogLabel(const VisualCatalogEntry& entry);
wxString CurrentValueLabel(const VisualRule& rule);
bool IsNeutralColor(const wxColour& color);
void StyleModeButton(wxButton* button, bool active);

struct TreeItemData final : public wxTreeItemData {
	enum class Kind {
		Group,
		Rule,
	};

	TreeItemData(Kind item_kind, std::string item_key = {}, std::string item_group = {});

	Kind kind;
	std::string key;
	std::string group;
};

}

#endif
