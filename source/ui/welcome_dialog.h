#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <wx/wx.h>
#include <vector>

// Forward declarations
class wxListCtrl;
class wxImageList;

wxDECLARE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class WelcomeDialog : public wxDialog {
public:
	WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles);
	~WelcomeDialog();

private:
	// Event Handlers
	void OnButtonClicked(wxCommandEvent& event);
	void OnRecentFileActivated(wxCommandEvent& event);
	void OnRecentFileSelected(wxCommandEvent& event);

	void AddInfoField(wxSizer* sizer, wxWindow* parent, const wxString& label, const wxString& value, const wxString& artId, const wxColour& valCol = wxColour(0, 0, 0));

	wxImageList* m_imageList;
	wxListCtrl* m_recentList;
};

#endif // WELCOME_DIALOG_H
