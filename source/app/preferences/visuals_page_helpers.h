#ifndef RME_PREFERENCES_VISUALS_PAGE_HELPERS_H_
#define RME_PREFERENCES_VISUALS_PAGE_HELPERS_H_

#include "app/visuals.h"

#include <string>

class wxBitmapButton;
class wxColour;
class wxStaticText;

namespace VisualsPageHelpers {

std::string LowercaseCopy(std::string value);
bool IsNeutralColor(const wxColour& color);
wxString BuildBadgeLabel(bool has_override, bool invalid);
void StyleActionButton(wxBitmapButton* button, bool enabled, bool active);
void StyleBadge(wxStaticText* label, bool invalid);

}

#endif
