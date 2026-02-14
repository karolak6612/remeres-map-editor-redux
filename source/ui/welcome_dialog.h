#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <wx/wx.h>
#include <vector>

wxDECLARE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class WelcomeCanvas;

class WelcomeDialog : public wxDialog {
public:
	WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles);
	~WelcomeDialog() override;

	// Event Handlers
	void OnButtonClicked(wxCommandEvent& event);
	void OnCheckboxClicked(wxCommandEvent& event);
	void OnRecentFileClicked(wxCommandEvent& event);
	void OnCanvasAction(wxCommandEvent& event);

private:
	WelcomeCanvas* m_canvas;
};

#endif // WELCOME_DIALOG_H
