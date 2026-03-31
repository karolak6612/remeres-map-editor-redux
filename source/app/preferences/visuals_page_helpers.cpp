#include "app/preferences/visuals_page_helpers.h"

#include <ranges>

#include <wx/bmpbuttn.h>
#include <wx/stattext.h>

namespace VisualsPageHelpers {

std::string LowercaseCopy(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

bool IsNeutralColor(const wxColour& color) {
	return color.IsOk() && color.Red() == 255 && color.Green() == 255 && color.Blue() == 255 && color.Alpha() == 255;
}

wxString BuildBadgeLabel(bool has_override, bool invalid) {
	if (invalid) {
		return has_override ? wxString("OVR  INVALID") : wxString("INVALID");
	}
	return has_override ? wxString("OVR") : wxString {};
}

void StyleActionButton(wxBitmapButton* button, bool enabled, bool active) {
	if (!button) {
		return;
	}

	button->Enable(enabled);
	button->SetBackgroundColour(active ? wxColour(70, 104, 168) : enabled ? wxColour(54, 54, 54) : wxColour(88, 88, 88));
	button->SetForegroundColour(*wxWHITE);
	button->Refresh();
}

void StyleBadge(wxStaticText* label, bool invalid) {
	if (!label) {
		return;
	}

	label->SetForegroundColour(invalid ? wxColour(214, 92, 92) : wxColour(132, 132, 132));
}

}
