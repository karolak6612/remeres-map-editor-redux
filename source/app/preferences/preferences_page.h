#ifndef RME_PREFERENCES_PAGE_H
#define RME_PREFERENCES_PAGE_H

#include <wx/panel.h>

class PreferencesPage : public wxPanel {
public:
	PreferencesPage(wxWindow* parent) : wxPanel(parent, wxID_ANY) { }
	virtual ~PreferencesPage() = default;

	virtual void Apply() = 0;
	virtual void RestoreDefaults() { };
};

#endif
