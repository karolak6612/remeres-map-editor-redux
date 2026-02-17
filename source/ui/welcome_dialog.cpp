#include "app/main.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "ui/controls/modern_button.h"
#include "util/image_manager.h"
#include <wx/dcbuffer.h>
#include <wx/statline.h>
#include "ui/controls/virtual_list_canvas.h"

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class WelcomeCanvas : public VirtualListCanvas {
public:
	WelcomeCanvas(wxWindow* parent, const std::vector<wxString>& recentFiles) :
		VirtualListCanvas(parent, wxID_ANY),
		m_files(recentFiles) {
		SetItemHeight(50);
		SetSelectionMode(SelectionMode::Single);
	}

	size_t GetItemCount() const override {
		return m_files.size();
	}

	void OnDrawItem(NVGcontext* vg, int index, const wxRect& rect) override {
		if (index < 0 || index >= (int)m_files.size()) return;

		wxFileName fn(m_files[index]);
		wxString filename = fn.GetFullName();
		wxString fpath = fn.GetPath();

		// Background handled by VirtualListCanvas logic

		// Filename
		nvgFontFace(vg, "sans-bold"); // Assuming sans-bold exists, otherwise falls back
		nvgFontSize(vg, 16.0f);

		NVGcolor textColor = m_textColor;
		if (IsSelected(index)) {
			textColor = nvgRGBA(255, 255, 255, 255);
		}

		nvgFillColor(vg, textColor);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, rect.GetX() + 12, rect.GetY() + 6, filename.c_str(), nullptr);

		// Path
		nvgFontFace(vg, "sans");
		nvgFontSize(vg, 12.0f);
		NVGcolor gray = nvgRGBA(128, 128, 128, 255);
		if (IsSelected(index)) gray = nvgRGBA(200, 200, 200, 255);
		nvgFillColor(vg, gray);

		nvgText(vg, rect.GetX() + 12, rect.GetY() + 24, fpath.c_str(), nullptr);
	}

	wxString GetFilePath(int index) const {
		if (index >= 0 && index < (int)m_files.size()) return m_files[index];
		return "";
	}

private:
	std::vector<wxString> m_files;
};

// Main Panel Class
class WelcomeDialog::WelcomePanel : public wxPanel {
public:
	WelcomePanel(WelcomeDialog* parent, const wxBitmap& logo, const std::vector<wxString>& recentFiles) :
		wxPanel(parent) {
		SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

		wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

		// --- Left Sidebar (System Shaded) ---
		wxPanel* sidebar = new wxPanel(this);
		sidebar->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
		wxBoxSizer* sideSizer = new wxBoxSizer(wxVERTICAL);

		// Logo
		wxStaticBitmap* logoCtrl = new wxStaticBitmap(sidebar, wxID_ANY, logo);
		sideSizer->Add(logoCtrl, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxBOTTOM, FromDIP(20));

		// Title
		wxStaticText* title = new wxStaticText(sidebar, wxID_ANY, "RME");
		title->SetFont(wxFontInfo(16).Bold());
		title->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		sideSizer->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(15));

		// Branding Info (Version) - Discrete
		wxStaticText* version = new wxStaticText(sidebar, wxID_ANY, wxString("v") << __W_RME_VERSION__);
		version->SetFont(wxFontInfo(7));
		version->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
		sideSizer->Add(version, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(25));

		// Actions (Nav-style)
		auto addButton = [&](const wxString& label, int id, const char* icon) {
			ModernButton* btn = new ModernButton(sidebar, id, label);
			btn->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
			btn->SetMinSize(wxSize(-1, FromDIP(35))); // More compact nav height
			if (icon) {
				btn->SetBitmap(IMAGE_MANAGER.GetBitmap(icon, wxSize(16, 16)));
			}
			sideSizer->Add(btn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
			btn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, parent);
		};

		addButton("New Project", wxID_NEW, ICON_NEW);
		addButton("Open Project", wxID_OPEN, ICON_OPEN);
		addButton("Preferences", wxID_PREFERENCES, ICON_GEAR);

		sideSizer->AddStretchSpacer();

		sidebar->SetSizer(sideSizer);

		// --- Right Content (Recent Files) ---
		wxPanel* content = new wxPanel(this);
		content->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

		wxStaticText* recentTitle = new wxStaticText(content, wxID_ANY, "Recent Projects");
		recentTitle->SetFont(wxFontInfo(12).Bold());
		recentTitle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		contentSizer->Add(recentTitle, 0, wxLEFT | wxTOP | wxBOTTOM, FromDIP(15));

		// Virtual List Canvas
		WelcomeCanvas* canvas = new WelcomeCanvas(content, recentFiles);
		contentSizer->Add(canvas, 1, wxEXPAND);

		// Bind selection event
		canvas->Bind(wxEVT_LISTBOX, [parent, canvas](wxCommandEvent& evt) {
			int sel = canvas->GetSelection();
			if (sel != wxNOT_FOUND) {
				wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
				newEvent->SetId(wxID_OPEN);
				newEvent->SetString(canvas->GetFilePath(sel));
				parent->QueueEvent(newEvent);
			}
		});

		// Anchored Footer for Startup Toggle
		wxPanel* footer = new wxPanel(content);
		footer->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		wxBoxSizer* footerSizer = new wxBoxSizer(wxHORIZONTAL);

		wxCheckBox* startupCheck = new wxCheckBox(footer, wxID_ANY, "Show on Startup");
		startupCheck->SetFont(wxFontInfo(8));
		startupCheck->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
		startupCheck->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) != 0);
		startupCheck->Bind(wxEVT_CHECKBOX, &WelcomeDialog::OnCheckboxClicked, parent);
		footerSizer->Add(startupCheck, 0, wxALL, FromDIP(10));

		footer->SetSizer(footerSizer);
		contentSizer->Add(footer, 0, wxEXPAND | wxTOP, 0);

		content->SetSizer(contentSizer);

		// combine
		mainSizer->Add(sidebar, 0, wxEXPAND | wxRIGHT, 0);
		mainSizer->Add(content, 1, wxEXPAND);

		SetSizer(mainSizer);
	}
};

WelcomeDialog::WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles) :
	wxDialog(nullptr, wxID_ANY, titleText, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
	Centre();
	new WelcomePanel(this, rmeLogo, recentFiles);
	SetSize(FromDIP(800), FromDIP(500));
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
		// Open file dialog
		wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";

		wxString filePath;
		{
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
