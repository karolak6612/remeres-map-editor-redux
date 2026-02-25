#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include <string_view>
#include <memory>

#include "ui/controls/recent_file_listbox.h"

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
	void OnRecentFileSelected(wxCommandEvent& event); // Changed event type

	void AddInfoField(wxSizer* sizer, wxWindow* parent, const wxString& label, const wxString& value, std::string_view artId, const wxColour& valCol = wxNullColour);

	wxPanel* CreateHeaderPanel(wxWindow* parent, const wxString& titleText, const wxBitmap& rmeLogo);
	wxPanel* CreateContentPanel(wxWindow* parent, const std::vector<wxString>& recentFiles);
	wxPanel* CreateFooterPanel(wxWindow* parent, const wxString& versionText);

	std::unique_ptr<wxImageList> m_imageList;
	RecentFileListBox* m_recentList = nullptr; // Changed type
	wxListCtrl* m_clientList = nullptr;
};

#endif // WELCOME_DIALOG_H
