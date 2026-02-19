#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <wx/wx.h>
#include <wx/listctrl.h>
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
	void OnRecentFileActivated(wxListEvent& event);
	void OnRecentFileSelected(wxListEvent& event);

	void AddInfoField(wxSizer* sizer, wxWindow* parent, const wxString& label, const wxString& value, const wxString& iconPath, const wxColour& valCol = wxNullColour);

	wxPanel* CreateHeaderPanel(wxWindow* parent, const wxString& titleText, const wxBitmap& rmeLogo);
	wxPanel* CreateContentPanel(wxWindow* parent, const std::vector<wxString>& recentFiles);
	wxPanel* CreateFooterPanel(wxWindow* parent, const wxString& versionText);

	wxImageList* m_imageList = nullptr;
	wxListCtrl* m_recentList = nullptr;
	wxListCtrl* m_clientList = nullptr;
};

#endif // WELCOME_DIALOG_H
