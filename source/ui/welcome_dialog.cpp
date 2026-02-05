#include "app/main.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "ui/controls/modern_button.h"
#include "util/nanovg_canvas.h"
#include <wx/dcbuffer.h>
#include <wx/statline.h>

#include <nanovg.h>

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class RecentFilesView : public NanoVGCanvas {
public:
	RecentFilesView(wxWindow* parent, const std::vector<wxString>& recentFiles) :
		NanoVGCanvas(parent, wxID_ANY),
		m_files(recentFiles),
		m_hoverIndex(-1),
		m_itemHeight(50) {

		Bind(wxEVT_MOTION, &RecentFilesView::OnMotion, this);
		Bind(wxEVT_LEAVE_WINDOW, &RecentFilesView::OnLeave, this);
		Bind(wxEVT_LEFT_DOWN, &RecentFilesView::OnLeftDown, this);
		Bind(wxEVT_SIZE, &RecentFilesView::OnSize, this);

		UpdateLayout();
	}

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override {
		int scrollPos = GetScrollPosition();
		int startRow = scrollPos / m_itemHeight;
		int endRow = (scrollPos + height + m_itemHeight - 1) / m_itemHeight + 1;

		int startIdx = std::max(0, startRow);
		int endIdx = std::min(static_cast<int>(m_files.size()), endRow);

		// Fill background
		nvgBeginPath(vg);
		nvgRect(vg, 0, scrollPos, width, height);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // White background
		nvgFill(vg);

		for (int i = startIdx; i < endIdx; ++i) {
			int y = i * m_itemHeight;

			// Hover effect
			if (i == m_hoverIndex) {
				nvgBeginPath(vg);
				nvgRect(vg, 0, y, width, m_itemHeight);
				nvgFillColor(vg, nvgRGBA(240, 240, 245, 255)); // Light highlight
				nvgFill(vg);
			}

			// File Name
			wxFileName fn(m_files[i]);
			wxString filename = fn.GetFullName();
			wxString fpath = fn.GetPath();

			nvgFontSize(vg, 14.0f);
			nvgFontFace(vg, "sans-bold");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
			nvgText(vg, 12, y + 8, filename.ToUTF8().data(), nullptr);

			// File Path
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgFillColor(vg, nvgRGBA(128, 128, 128, 255));
			nvgText(vg, 12, y + 26, fpath.ToUTF8().data(), nullptr);

			// Separator line (optional, but nice)
			nvgBeginPath(vg);
			nvgMoveTo(vg, 12, y + m_itemHeight - 1);
			nvgLineTo(vg, width - 12, y + m_itemHeight - 1);
			nvgStrokeColor(vg, nvgRGBA(240, 240, 240, 255));
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}
	}

	void UpdateLayout() {
		int contentHeight = m_files.size() * m_itemHeight;
		UpdateScrollbar(contentHeight);
	}

	void OnSize(wxSizeEvent& event) {
		UpdateLayout();
		Refresh();
		event.Skip();
	}

	void OnMotion(wxMouseEvent& event) {
		int y = event.GetY() + GetScrollPosition();
		int index = y / m_itemHeight;

		if (index >= 0 && index < static_cast<int>(m_files.size())) {
			if (index != m_hoverIndex) {
				m_hoverIndex = index;
				SetCursor(wxCursor(wxCURSOR_HAND));
				Refresh();
			}
		} else {
			if (m_hoverIndex != -1) {
				m_hoverIndex = -1;
				SetCursor(wxNullCursor);
				Refresh();
			}
		}
	}

	void OnLeave(wxMouseEvent& event) {
		m_hoverIndex = -1;
		SetCursor(wxNullCursor);
		Refresh();
	}

	void OnLeftDown(wxMouseEvent& event) {
		int y = event.GetY() + GetScrollPosition();
		int index = y / m_itemHeight;

		if (index >= 0 && index < static_cast<int>(m_files.size())) {
			// Trigger wxEVT_BUTTON to match legacy behavior which parent listens to
			wxCommandEvent evt(wxEVT_BUTTON, GetId());
			evt.SetString(m_files[index]);
			evt.SetEventObject(this);

			// Bubble up to parent (WelcomePanel -> WelcomeDialog)
			// But since we are binding explicitly in WelcomePanel, emitting on self is enough
			GetEventHandler()->ProcessEvent(evt);
		}
	}

	wxSize DoGetBestClientSize() const override {
		return wxSize(FromDIP(300), FromDIP(200));
	}

private:
	std::vector<wxString> m_files;
	int m_hoverIndex;
	int m_itemHeight;
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
		auto addButton = [&](const wxString& label, int id) {
			ModernButton* btn = new ModernButton(sidebar, id, label);
			btn->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
			btn->SetMinSize(wxSize(-1, FromDIP(35))); // More compact nav height
			sideSizer->Add(btn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
			btn->Bind(wxEVT_BUTTON, &WelcomeDialog::OnButtonClicked, parent);
		};

		addButton("New Project", wxID_NEW);
		addButton("Open Project", wxID_OPEN);
		addButton("Preferences", wxID_PREFERENCES);

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

		// NanoVG Recent Files View
		RecentFilesView* recentView = new RecentFilesView(content, recentFiles);
		contentSizer->Add(recentView, 1, wxEXPAND | wxBOTTOM, 1);

		// Bind event to parent dialog handler
		recentView->Bind(wxEVT_BUTTON, &WelcomeDialog::OnRecentFileClicked, parent);

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
		wxCommandEvent newEvent(WELCOME_DIALOG_ACTION);
		newEvent.SetId(wxID_NEW);
		ProcessWindowEvent(newEvent);
	} else if (id == wxID_OPEN) {
		// Open file dialog
		wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? "(*.otbm;*.otgz)|*.otbm;*.otgz" : "(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";

		wxFileDialog file_dialog(this, "Open map file", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (file_dialog.ShowModal() == wxID_OK) {
			wxCommandEvent newEvent(WELCOME_DIALOG_ACTION);
			newEvent.SetId(wxID_OPEN);
			newEvent.SetString(file_dialog.GetPath());
			ProcessWindowEvent(newEvent);
		}
	}
}

void WelcomeDialog::OnCheckboxClicked(wxCommandEvent& event) {
	g_settings.setInteger(Config::WELCOME_DIALOG, event.IsChecked() ? 1 : 0);
}

void WelcomeDialog::OnRecentFileClicked(wxCommandEvent& event) {
	wxCommandEvent newEvent(WELCOME_DIALOG_ACTION);
	newEvent.SetId(wxID_OPEN);
	newEvent.SetString(event.GetString());
	ProcessWindowEvent(newEvent);
}
