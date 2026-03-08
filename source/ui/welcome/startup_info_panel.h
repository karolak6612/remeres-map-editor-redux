#ifndef RME_STARTUP_INFO_PANEL_H_
#define RME_STARTUP_INFO_PANEL_H_

#include <vector>

#include <wx/wx.h>

#include "ui/welcome/startup_types.h"

class StartupInfoPanel : public wxPanel {
public:
	explicit StartupInfoPanel(wxWindow* parent);

	void SetFields(const std::vector<StartupInfoField>& fields);

private:
	void OnSize(wxSizeEvent& event);
	void UpdateWrapping();

	wxBoxSizer* m_content_sizer = nullptr;
	std::vector<wxStaticText*> m_value_labels;
};

#endif
