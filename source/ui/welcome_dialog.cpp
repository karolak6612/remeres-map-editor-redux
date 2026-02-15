#include "app/main.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "ui/welcome_canvas.h"
#include "util/image_manager.h"

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

WelcomeDialog::WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles) :
	wxDialog(nullptr, wxID_ANY, titleText, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
	Centre();

	WelcomeCanvas* canvas = newd WelcomeCanvas(this, rmeLogo, recentFiles);
	wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	sizer->Add(canvas, 1, wxEXPAND);
	SetSizer(sizer);

	SetSize(FromDIP(800), FromDIP(500));

	Bind(WELCOME_DIALOG_ACTION, &WelcomeDialog::OnButtonClicked, this);
}

void WelcomeDialog::OnButtonClicked(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == wxID_PREFERENCES) {
		PreferencesWindow preferences_window(this, true);
		preferences_window.ShowModal();
		// Update UI if needed
	} else if (id == wxID_NEW) {
		wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
		newEvent->SetId(wxID_NEW);
		QueueEvent(newEvent);
	} else if (id == wxID_OPEN) {
		wxString filePath = event.GetString();

		if (filePath.IsEmpty()) {
			// Open file dialog
			wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";

			wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (file_dialog.ShowModal() == wxID_OK) {
				filePath = file_dialog.GetPath();
			}
		}

		if (!filePath.IsEmpty()) {
			wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
			newEvent->SetId(wxID_OPEN);
			newEvent->SetString(filePath);
			QueueEvent(newEvent);
		}
	}
}

void WelcomeDialog::OnCheckboxClicked(wxCommandEvent& event) {
	g_settings.setInteger(Config::WELCOME_DIALOG, event.IsChecked() ? 1 : 0);
}

void WelcomeDialog::OnRecentFileClicked(wxCommandEvent& event) {
	wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
	newEvent->SetId(wxID_OPEN);
	newEvent->SetString(event.GetString());
	QueueEvent(newEvent);
}
