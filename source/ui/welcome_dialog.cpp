#include "app/main.h"
#include "ui/welcome_dialog.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "ui/controls/modern_button.h"
#include <wx/dcbuffer.h>
#include <wx/statline.h>

#include "util/nanovg_canvas.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

// Define WelcomeCanvas
class WelcomeDialog::WelcomeCanvas : public NanoVGCanvas {
public:
	WelcomeCanvas(WelcomeDialog* parent, const wxBitmap& logo, const std::vector<wxString>& recentFiles) :
		NanoVGCanvas(parent, wxID_ANY, 0 | wxWANTS_CHARS),
		m_parent(parent),
		m_logo(logo),
		m_recentFiles(recentFiles) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);

		Bind(wxEVT_LEFT_DOWN, &WelcomeCanvas::OnMouseDown, this);
		Bind(wxEVT_MOTION, &WelcomeCanvas::OnMouseMove, this);
		Bind(wxEVT_LEAVE_WINDOW, &WelcomeCanvas::OnMouseLeave, this);
		Bind(wxEVT_LEFT_UP, &WelcomeCanvas::OnMouseUp, this);
	}

	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override {
		// Modern Dark Theme Colors
		NVGcolor bgCol = nvgRGBA(30, 32, 36, 255);
		NVGcolor sidebarCol = nvgRGBA(24, 25, 28, 255);
		NVGcolor textCol = nvgRGBA(220, 220, 220, 255);

		// Background
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, width, height);
		nvgFillColor(vg, bgCol);
		nvgFill(vg);

		// Sidebar
		float sidebarW = 250.0f;
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, sidebarW, height);
		nvgFillColor(vg, sidebarCol);
		nvgFill(vg);

		// Logo
		int logoTex = GetLogoTexture(vg);
		if (logoTex > 0) {
			float logoSize = 128.0f; // Adjust based on actual logo
			float logoX = (sidebarW - logoSize) / 2;
			float logoY = 40.0f;
			NVGpaint imgPaint = nvgImagePattern(vg, logoX, logoY, logoSize, logoSize, 0.0f, logoTex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, logoX, logoY, logoSize, logoSize);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		// Sidebar Buttons
		DrawButton(vg, GetSidebarButtonRect(0), "New Project", wxID_NEW);
		DrawButton(vg, GetSidebarButtonRect(1), "Open Project", wxID_OPEN);
		DrawButton(vg, GetSidebarButtonRect(2), "Preferences", wxID_PREFERENCES);

		// Version
		std::string version = "v" + std::string(__W_RME_VERSION__);
		nvgFontSize(vg, 12.0f);
		nvgFillColor(vg, nvgRGBA(100, 100, 100, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
		nvgText(vg, sidebarW / 2, height - 20.0f, version.c_str(), nullptr);

		// Recent Projects Area
		float contentX = sidebarW + 40.0f;
		float contentY = 40.0f;

		nvgFontSize(vg, 24.0f);
		nvgFillColor(vg, textCol);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, contentX, contentY, "Recent Projects", nullptr);

		for (size_t i = 0; i < m_recentFiles.size(); ++i) {
			wxRect itemRect = GetRecentItemRect(static_cast<int>(i), width);
			if (itemRect.GetBottom() > height) break; // Clip

			DrawRecentItem(vg, itemRect, m_recentFiles[i], static_cast<int>(i));
		}

		// Startup Checkbox
		float contentX = 250.0f + 40.0f;
		float checkboxY = height - 40.0f;
		bool checked = g_settings.getInteger(Config::WELCOME_DIALOG) != 0;
		DrawCheckbox(vg, wxRect(contentX, checkboxY, 200, 20), "Show on Startup", checked, (m_hoverId == 2000));
	}

	void DrawCheckbox(NVGcontext* vg, const wxRect& r, const std::string& label, bool checked, bool hover) {
		float boxSz = 16.0f;
		float boxX = r.x;
		float boxY = r.y + (r.height - boxSz) / 2;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, boxX, boxY, boxSz, boxSz, 4.0f);
		if (checked) {
			nvgFillColor(vg, nvgRGBA(100, 150, 250, 255));
		} else {
			nvgFillColor(vg, nvgRGBA(60, 60, 65, 255));
		}
		nvgFill(vg);

		if (hover) {
			nvgStrokeColor(vg, nvgRGBA(150, 200, 255, 255));
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}

		if (checked) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, boxX + 4, boxY + 8);
			nvgLineTo(vg, boxX + 7, boxY + 11);
			nvgLineTo(vg, boxX + 12, boxY + 5);
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		}

		nvgFontSize(vg, 14.0f);
		nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, boxX + boxSz + 10.0f, r.y + r.height / 2, label.c_str(), nullptr);
	}

	int GetLogoTexture(NVGcontext* vg) {
		if (m_logoTex > 0) return m_logoTex;
		if (!m_logo.IsOk()) return 0;

		wxImage img = m_logo.ConvertToImage();
		if (!img.IsOk()) return 0;

		int w = img.GetWidth();
		int h = img.GetHeight();
		std::vector<uint8_t> data(w * h * 4);

		uint8_t* rgb = img.GetData();
		uint8_t* alpha = img.HasAlpha() ? img.GetAlpha() : nullptr;

		for (int i = 0; i < w * h; ++i) {
			data[i * 4 + 0] = rgb[i * 3 + 0];
			data[i * 4 + 1] = rgb[i * 3 + 1];
			data[i * 4 + 2] = rgb[i * 3 + 2];
			data[i * 4 + 3] = alpha ? alpha[i] : 255;
		}

		m_logoTex = nvgCreateImageRGBA(vg, w, h, 0, data.data());
		return m_logoTex;
	}

	void DrawButton(NVGcontext* vg, const wxRect& r, const std::string& label, int id) {
		bool hover = (m_hoverId == id);
		bool pressed = (m_pressedId == id);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.x, r.y, r.width, r.height, 6.0f);

		if (pressed) {
			nvgFillColor(vg, nvgRGBA(60, 90, 160, 255));
		} else if (hover) {
			nvgFillColor(vg, nvgRGBA(50, 55, 65, 255));
		} else {
			nvgFillColor(vg, nvgRGBA(40, 42, 48, 255)); // Button base
		}
		nvgFill(vg);

		if (hover) {
			nvgStrokeColor(vg, nvgRGBA(100, 150, 250, 100));
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}

		nvgFontSize(vg, 16.0f);
		nvgFillColor(vg, nvgRGBA(230, 230, 230, 255));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, r.x + r.width / 2, r.y + r.height / 2, label.c_str(), nullptr);
	}

	void DrawRecentItem(NVGcontext* vg, const wxRect& r, const wxString& path, int index) {
		int id = 1000 + index; // ID offset for recent items
		bool hover = (m_hoverId == id);
		bool pressed = (m_pressedId == id);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.x, r.y, r.width, r.height, 6.0f);

		if (pressed) {
			nvgFillColor(vg, nvgRGBA(40, 45, 55, 255));
		} else if (hover) {
			nvgFillColor(vg, nvgRGBA(40, 42, 48, 255));
		} else {
			// Transparent / subtle
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 5));
		}
		nvgFill(vg);

		// Icon placeholder
		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.x + 10, r.y + 10, 40, 40, 4.0f);
		nvgFillColor(vg, nvgRGBA(60, 60, 65, 255));
		nvgFill(vg);

		// Map Icon
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 20, r.y + 20, 20, 20);
		nvgFillColor(vg, nvgRGBA(100, 150, 250, 200));
		nvgFill(vg);

		// Text
		wxFileName fn(path);
		std::string name = fn.GetFullName().ToStdString();
		std::string dir = fn.GetPath().ToStdString();

		nvgFontSize(vg, 16.0f);
		nvgFillColor(vg, nvgRGBA(240, 240, 240, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, r.x + 65, r.y + 12, name.c_str(), nullptr);

		nvgFontSize(vg, 12.0f);
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		nvgText(vg, r.x + 65, r.y + 35, dir.c_str(), nullptr);
	}

	void OnMouseMove(wxMouseEvent& evt) {
		wxPoint pos = evt.GetPosition();
		int prevHover = m_hoverId;
		m_hoverId = HitTest(pos);
		if (prevHover != m_hoverId) {
			Refresh();
		}
	}

	void OnMouseLeave(wxMouseEvent& evt) {
		if (m_hoverId != -1) {
			m_hoverId = -1;
			Refresh();
		}
	}

	void OnMouseDown(wxMouseEvent& evt) {
		m_pressedId = HitTest(evt.GetPosition());
		if (m_pressedId != -1) {
			CaptureMouse();
			Refresh();
		}
	}

	void OnMouseUp(wxMouseEvent& evt) {
		if (HasCapture()) {
			ReleaseMouse();
		}

		int hit = HitTest(evt.GetPosition());
		if (hit == m_pressedId && hit != -1) {
			// Action
			if (hit == 2000) {
				bool current = g_settings.getInteger(Config::WELCOME_DIALOG) != 0;
				g_settings.setInteger(Config::WELCOME_DIALOG, !current ? 1 : 0);
			} else if (hit >= 1000) {
				// Recent File
				int idx = hit - 1000;
				if (idx >= 0 && idx < m_recentFiles.size()) {
					wxCommandEvent event(WELCOME_DIALOG_ACTION);
					event.SetId(wxID_OPEN); // Reusing OPEN id but passing string
					event.SetString(m_recentFiles[idx]);
					event.SetEventObject(this);
					m_parent->GetEventHandler()->ProcessEvent(event);
				}
			} else {
				// Button
				wxCommandEvent event(wxEVT_BUTTON, hit);
				event.SetEventObject(m_parent);
				m_parent->OnButtonClicked(event);
			}
		}
		m_pressedId = -1;
		Refresh();
	}

	int HitTest(const wxPoint& pt) {
		if (GetSidebarButtonRect(0).Contains(pt)) return wxID_NEW;
		if (GetSidebarButtonRect(1).Contains(pt)) return wxID_OPEN;
		if (GetSidebarButtonRect(2).Contains(pt)) return wxID_PREFERENCES;

		int w = GetClientSize().x;
		int h = GetClientSize().y;
		for (size_t i = 0; i < m_recentFiles.size(); ++i) {
			wxRect r = GetRecentItemRect(static_cast<int>(i), w);
			if (r.GetBottom() > h) break;
			if (r.Contains(pt)) return 1000 + static_cast<int>(i);
		}

		float contentX = 250.0f + 40.0f;
		float checkboxY = h - 40.0f;
		if (wxRect(contentX, checkboxY, 200, 20).Contains(pt)) return 2000;

		return -1;
	}

private:
	WelcomeDialog* m_parent;
	wxBitmap m_logo;
	std::vector<wxString> m_recentFiles;
	int m_logoTex = 0;
	int m_hoverId = -1;
	int m_pressedId = -1;

	wxRect GetSidebarButtonRect(int index) const {
		float sidebarW = 250.0f;
		float btnY = 200.0f;
		float btnH = 40.0f;
		float btnW = sidebarW - 40.0f;
		float btnX = 20.0f;
		float gap = 10.0f;
		return wxRect(btnX, btnY + index * (btnH + gap), btnW, btnH);
	}

	wxRect GetRecentItemRect(int index, int width) const {
		float sidebarW = 250.0f;
		float contentX = sidebarW + 40.0f;
		float contentY = 40.0f;
		float contentW = width - contentX - 40.0f;
		float listY = contentY + 50.0f;
		float itemH = 60.0f;
		float gap = 10.0f;
		return wxRect(contentX, listY + index * (itemH + gap), contentW, itemH);
	}
};

WelcomeDialog::WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles) :
	wxDialog(nullptr, wxID_ANY, titleText, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
	Centre();
	SetBackgroundColour(wxColour(30, 32, 36)); // Match NanoVG bg

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	WelcomeCanvas* canvas = new WelcomeCanvas(this, rmeLogo, recentFiles);
	sizer->Add(canvas, 1, wxEXPAND);
	SetSizer(sizer);

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

void WelcomeDialog::OnCanvasAction(wxCommandEvent& event) {
	// If the canvas triggers actions directly, we handle them here
	// Currently the canvas calls OnButtonClicked or emits WELCOME_DIALOG_ACTION directly
}
