#ifndef RME_PREFERENCES_LAYOUT_H_
#define RME_PREFERENCES_LAYOUT_H_

#include <vector>

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "ui/theme.h"

class PreferencesSectionPanel : public wxPanel {
public:
	PreferencesSectionPanel(wxWindow* parent, const wxString& title, const wxString& description = wxString());

	wxBoxSizer* GetBodySizer() const {
		return m_body_sizer;
	}

	void RegisterWrappedLabel(wxStaticText* label);

private:
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void UpdateWrapping();

	wxStaticText* m_description_label = nullptr;
	wxBoxSizer* m_body_sizer = nullptr;
	std::vector<wxStaticText*> m_wrapped_labels;
};

namespace PreferencesLayout {
wxStaticText* CreateBodyText(wxWindow* parent, const wxString& text, bool bold = false);
wxStaticText* AddNotice(PreferencesSectionPanel* section, const wxString& text, Theme::Role role = Theme::Role::TextSubtle);
wxCheckBox* AddCheckBoxRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	bool value
);
void AddControlRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	wxWindow* control,
	bool expand_control = false
);
wxStaticText* AddValuePreviewRow(
	PreferencesSectionPanel* section,
	const wxString& title,
	const wxString& description,
	const wxString& value
);
}

#endif
