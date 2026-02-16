#include "app/main.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "ui/controls/modern_button.h"
#include "util/image_manager.h"
#include "util/nanovg_canvas.h"
#include <wx/dcbuffer.h>
#include <wx/statline.h>

#include <glad/glad.h>
#include <nanovg.h>

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class WelcomeCanvas : public NanoVGCanvas {
public:
	WelcomeCanvas(wxWindow* parent, const std::vector<wxString>& recentFiles) :
		NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
		m_recentFiles(recentFiles),
		m_hoverIndex(-1),
		m_pressedIndex(-1),
		m_itemHeight(50) {

		Bind(wxEVT_SIZE, &WelcomeCanvas::OnSize, this);
		Bind(wxEVT_MOTION, &WelcomeCanvas::OnMotion, this);
		Bind(wxEVT_LEFT_DOWN, &WelcomeCanvas::OnMouseDown, this);
		Bind(wxEVT_LEFT_UP, &WelcomeCanvas::OnMouseUp, this);
		Bind(wxEVT_LEAVE_WINDOW, &WelcomeCanvas::OnLeave, this);

		UpdateLayout();
	}

	void UpdateLayout() {
		int rows = static_cast<int>(m_recentFiles.size());
		int contentHeight = rows * m_itemHeight;
		UpdateScrollbar(contentHeight);
	}

	void OnSize(wxSizeEvent& event) {
		UpdateLayout();
		Refresh();
		event.Skip();
	}

	wxSize DoGetBestClientSize() const override {
		return FromDIP(wxSize(400, 300));
	}

	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override {
		int scrollPos = GetScrollPosition();

		// Background
		wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		nvgBeginPath(vg);
		nvgRect(vg, 0, scrollPos, width, height);
		nvgFillColor(vg, nvgRGBA(bgCol.Red(), bgCol.Green(), bgCol.Blue(), 255));
		nvgFill(vg);

		int startRow = scrollPos / m_itemHeight;
		int endRow = (scrollPos + height + m_itemHeight - 1) / m_itemHeight;
		int count = static_cast<int>(m_recentFiles.size());

		int startIdx = std::max(0, startRow);
		int endIdx = std::min(count, endRow + 1);

		wxColour highlightCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		NVGcolor pressedColor = nvgRGBA(highlightCol.Red(), highlightCol.Green(), highlightCol.Blue(), 50); // Light pressed
		NVGcolor hoverColor = nvgRGBA(highlightCol.Red(), highlightCol.Green(), highlightCol.Blue(), 25); // Lighter hover

		wxColour textCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		NVGcolor textColor = nvgRGBA(textCol.Red(), textCol.Green(), textCol.Blue(), 255);

		wxColour grayCol = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
		NVGcolor grayColor = nvgRGBA(grayCol.Red(), grayCol.Green(), grayCol.Blue(), 255);

		wxColour borderCol = wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
		NVGcolor borderColor = nvgRGBA(borderCol.Red(), borderCol.Green(), borderCol.Blue(), 255);

		for (int i = startIdx; i < endIdx; ++i) {
			int y = i * m_itemHeight;

			// Background for hover/pressed
			if (i == m_pressedIndex) {
				nvgBeginPath(vg);
				nvgRect(vg, 0, y, width, m_itemHeight);
				nvgFillColor(vg, pressedColor);
				nvgFill(vg);
			} else if (i == m_hoverIndex) {
				nvgBeginPath(vg);
				nvgRect(vg, 0, y, width, m_itemHeight);
				nvgFillColor(vg, hoverColor);
				nvgFill(vg);
			}

			// Text
			wxFileName fn(m_recentFiles[i]);
			wxString filename = fn.GetFullName();
			wxString fpath = fn.GetPath();

			// Filename
			nvgFontSize(vg, 16.0f);
			nvgFontFace(vg, "sans-bold");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgFillColor(vg, textColor);
			nvgText(vg, 12, y + 8, filename.ToUTF8().data(), nullptr);

			// Path
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgFillColor(vg, grayColor);
			nvgText(vg, 12, y + 28, fpath.ToUTF8().data(), nullptr);

			// Separator line
			nvgBeginPath(vg);
			nvgMoveTo(vg, 0, y + m_itemHeight - 0.5f);
			nvgLineTo(vg, width, y + m_itemHeight - 0.5f);
			nvgStrokeColor(vg, borderColor);
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}
	}

	int HitTest(int x, int y) {
		int scrollPos = GetScrollPosition();
		int realY = y + scrollPos;
		int index = realY / m_itemHeight;
		if (index >= 0 && index < static_cast<int>(m_recentFiles.size())) {
			return index;
		}
		return -1;
	}

	void OnMotion(wxMouseEvent& event) {
		int index = HitTest(event.GetX(), event.GetY());
		if (index != m_hoverIndex) {
			m_hoverIndex = index;
			if (m_hoverIndex != -1) {
				SetCursor(wxCursor(wxCURSOR_HAND));
			} else {
				SetCursor(wxNullCursor);
			}
			Refresh();
		}
		event.Skip();
	}

	void OnLeave(wxMouseEvent& event) {
		if (m_hoverIndex != -1) {
			m_hoverIndex = -1;
			Refresh();
		}
		event.Skip();
	}

	void OnMouseDown(wxMouseEvent& event) {
		int index = HitTest(event.GetX(), event.GetY());
		if (index != -1) {
			m_pressedIndex = index;
			Refresh();
		}
		event.Skip();
	}

	void OnMouseUp(wxMouseEvent& event) {
		if (m_pressedIndex != -1) {
			int index = HitTest(event.GetX(), event.GetY());
			if (index == m_pressedIndex) {
				// Clicked!
				wxCommandEvent cmdEvent(wxEVT_BUTTON, GetId());
				cmdEvent.SetString(m_recentFiles[index]);
				cmdEvent.SetEventObject(this);

				// Process event on this control, allowing listeners (like bound in parent) to catch it.
				GetEventHandler()->ProcessEvent(cmdEvent);
			}
			m_pressedIndex = -1;
			Refresh();
		}
		event.Skip();
	}

private:
	std::vector<wxString> m_recentFiles;
	int m_hoverIndex;
	int m_pressedIndex;
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

		// WelcomeCanvas for recent files
		WelcomeCanvas* canvas = new WelcomeCanvas(content, recentFiles);

		// Bind event to parent dialog handler
		canvas->Bind(wxEVT_BUTTON, [parent](wxCommandEvent& evt) {
			parent->OnRecentFileClicked(evt);
		});

		contentSizer->Add(canvas, 1, wxEXPAND);

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

void WelcomeDialog::OnRecentFileClicked(wxCommandEvent& event) {
	wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
	newEvent->SetId(wxID_OPEN);
	newEvent->SetString(event.GetString());
	QueueEvent(newEvent);
}
