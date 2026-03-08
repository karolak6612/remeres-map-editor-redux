#ifndef RME_STARTUP_CARD_H_
#define RME_STARTUP_CARD_H_

#include <wx/dcbuffer.h>
#include <wx/wx.h>

class StartupCardPanel : public wxPanel {
public:
	StartupCardPanel(wxWindow* parent, const wxString& title);

	wxSizer* GetBodySizer() const {
		return m_body_sizer;
	}

private:
	void OnPaint(wxPaintEvent& event);

	wxString m_title;
	wxStaticText* m_title_label = nullptr;
	wxBoxSizer* m_body_sizer = nullptr;
};

#endif
