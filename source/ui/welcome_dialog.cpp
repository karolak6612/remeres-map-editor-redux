#include "ui/welcome_dialog.h"
#include "app/main.h"
#include "app/settings.h"
#include "app/preferences.h"
#include "util/image_manager.h"
#include "util/nanovg_canvas.h"
#include "ui/theme.h"
#include <wx/dcbuffer.h>
#include <wx/statline.h>
#include <nanovg.h>

wxDEFINE_EVENT(WELCOME_DIALOG_ACTION, wxCommandEvent);

class WelcomeCanvas : public NanoVGCanvas {
public:
	WelcomeCanvas(wxWindow* parent, const wxBitmap& logo, const std::vector<wxString>& recentFiles) :
		NanoVGCanvas(parent, wxID_ANY, wxWANTS_CHARS),
		m_logo(logo),
		m_recentFiles(recentFiles),
		m_logoTexture(0) {

		Bind(wxEVT_LEFT_DOWN, &WelcomeCanvas::OnMouse, this);
		Bind(wxEVT_LEFT_UP, &WelcomeCanvas::OnMouse, this);
		Bind(wxEVT_MOTION, &WelcomeCanvas::OnMouse, this);
		Bind(wxEVT_LEAVE_WINDOW, &WelcomeCanvas::OnLeave, this);
		Bind(wxEVT_SIZE, &WelcomeCanvas::OnSize, this);
		Bind(wxEVT_TIMER, &WelcomeCanvas::OnTimer, this);

		// Initialize layout
		RebuildLayout();
	}

	~WelcomeCanvas() override {
		if (m_logoTexture > 0) {
			NVGcontext* vg = GetNVGContext();
			if (vg) {
				nvgDeleteImage(vg, m_logoTexture);
			}
		}
	}

	void OnSize(wxSizeEvent& evt) {
		RebuildLayout();
		Refresh();
		evt.Skip();
	}

	void OnTimer(wxTimerEvent& evt) {
		// Animation
	}

	void OnLeave(wxMouseEvent& evt) {
		m_hoverBtn = -1;
		m_hoverFile = -1;
		m_hoverStartup = false;
		Refresh();
	}

	void OnMouse(wxMouseEvent& evt) {
		wxPoint pos = evt.GetPosition();
		int scroll = GetScrollPosition();

		bool inSidebar = (pos.x < m_sidebarWidth);

		// Buttons
		int prevBtn = m_hoverBtn;
		m_hoverBtn = -1;
		if (inSidebar) {
			for (size_t i = 0; i < m_buttons.size(); ++i) {
				if (m_buttons[i].rect.Contains(pos)) {
					m_hoverBtn = static_cast<int>(i);
					break;
				}
			}
		}

		// Recent Files (Scrolled)
		int prevFile = m_hoverFile;
		m_hoverFile = -1;
		if (!inSidebar) {
			wxPoint scrolledPos = pos;
			scrolledPos.y += scroll;
			for (size_t i = 0; i < m_fileRects.size(); ++i) {
				if (m_fileRects[i].Contains(scrolledPos)) {
					m_hoverFile = static_cast<int>(i);
					break;
				}
			}
		}

		// Startup Checkbox (Fixed at bottom right)
		bool prevStartup = m_hoverStartup;
		m_hoverStartup = false;
		if (!inSidebar) {
			if (m_startupRect.Contains(pos)) {
				m_hoverStartup = true;
			}
		}

		if (evt.LeftDown()) {
			if (m_hoverBtn != -1) {
				wxCommandEvent event(WELCOME_DIALOG_ACTION, GetId());
				event.SetId(m_buttons[m_hoverBtn].id);
				GetEventHandler()->ProcessEvent(event);
			} else if (m_hoverFile != -1) {
				wxCommandEvent event(WELCOME_DIALOG_ACTION, GetId());
				event.SetId(wxID_OPEN);
				event.SetString(m_recentFiles[m_hoverFile]);
				GetEventHandler()->ProcessEvent(event);
			} else if (m_hoverStartup) {
				int val = g_settings.getInteger(Config::WELCOME_DIALOG);
				g_settings.setInteger(Config::WELCOME_DIALOG, val ? 0 : 1);
				Refresh();
			}
		}

		if (prevBtn != m_hoverBtn || prevFile != m_hoverFile || prevStartup != m_hoverStartup) {
			Refresh();
		}

		bool hand = (m_hoverBtn != -1 || m_hoverFile != -1 || m_hoverStartup);
		SetCursor(hand ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));
	}

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override {
		int w = GetClientSize().x;
		int h = GetClientSize().y;
		int scroll = GetScrollPosition();

		// Draw Content (Scrolled)
		nvgBeginPath(vg);
		nvgRect(vg, m_sidebarWidth, 0, w - m_sidebarWidth, std::max(h, m_contentHeight));
		nvgFillColor(vg, nvgRGBA(240, 240, 245, 255));
		nvgFill(vg);

		nvgFontSize(vg, 24.0f);
		nvgFontFace(vg, "sans-bold");
		nvgFillColor(vg, nvgRGBA(50, 50, 60, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
		nvgText(vg, m_sidebarWidth + 30, 60, "Recent Projects", nullptr);

		for (size_t i = 0; i < m_recentFiles.size(); ++i) {
			DrawRecentFileCard(vg, i, m_fileRects[i]);
		}

		// Sidebar (Fixed)
		nvgSave(vg);
		nvgTranslate(vg, 0, scroll);

		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, m_sidebarWidth, h);
		nvgFillColor(vg, nvgRGBA(40, 44, 52, 255));
		nvgFill(vg);

		// Logo
		int tex = GetBitmapTexture(vg);
		if (tex > 0) {
			int imgW = m_logo.GetWidth();
			int imgH = m_logo.GetHeight();
			float scale = 1.0f;
			if (imgW > m_sidebarWidth - 40) {
				scale = (float)(m_sidebarWidth - 40) / imgW;
			}
			float lx = (m_sidebarWidth - imgW * scale) / 2.0f;
			float ly = 40.0f;

			NVGpaint imgPaint = nvgImagePattern(vg, lx, ly, imgW * scale, imgH * scale, 0.0f, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, lx, ly, imgW * scale, imgH * scale);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		} else {
			nvgFontSize(vg, 48.0f);
			nvgFontFace(vg, "sans-bold");
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgText(vg, m_sidebarWidth / 2, 80, "RME", nullptr);
		}

		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(150, 150, 160, 255));
		wxString ver = wxString::Format("v%s", __W_RME_VERSION__);
		nvgText(vg, m_sidebarWidth / 2, 140, ver.ToStdString().c_str(), nullptr);

		for (size_t i = 0; i < m_buttons.size(); ++i) {
			DrawSidebarButton(vg, m_buttons[i], (int)i == m_hoverBtn);
		}

		DrawStartupCheckbox(vg, m_startupRect, m_hoverStartup);

		nvgRestore(vg);
	}

private:
	struct ButtonDef {
		wxRect rect;
		wxString label;
		int id;
		const char* icon;
	};

	wxBitmap m_logo;
	std::vector<wxString> m_recentFiles;
	std::vector<wxRect> m_fileRects;
	std::vector<ButtonDef> m_buttons;
	wxRect m_startupRect;

	int m_sidebarWidth = 250;
	int m_contentHeight = 0;
	int m_hoverBtn = -1;
	int m_hoverFile = -1;
	bool m_hoverStartup = false;
	int m_logoTexture;

	int GetBitmapTexture(NVGcontext* vg) {
		if (m_logoTexture > 0) return m_logoTexture;
		if (!m_logo.IsOk()) return 0;

		wxImage img = m_logo.ConvertToImage();
		if (!img.IsOk()) return 0;

		int w = img.GetWidth();
		int h = img.GetHeight();
		std::vector<uint8_t> rgba(w * h * 4);
		const uint8_t* data = img.GetData();
		const uint8_t* alpha = img.GetAlpha();
		bool hasAlpha = img.HasAlpha();

		for (int i = 0; i < w * h; ++i) {
			rgba[i * 4 + 0] = data[i * 3 + 0];
			rgba[i * 4 + 1] = data[i * 3 + 1];
			rgba[i * 4 + 2] = data[i * 3 + 2];
			rgba[i * 4 + 3] = (hasAlpha && alpha) ? alpha[i] : 255;
		}

		m_logoTexture = nvgCreateImageRGBA(vg, w, h, 0, rgba.data());
		return m_logoTexture;
	}

	void RebuildLayout() {
		int w = GetClientSize().x;
		int h = GetClientSize().y;
		m_sidebarWidth = FromDIP(240);

		m_buttons.clear();
		int btnY = FromDIP(160);
		int btnH = FromDIP(50);

		auto addButton = [&](const wxString& label, int id) {
			ButtonDef b;
			b.rect = wxRect(0, btnY, m_sidebarWidth, btnH);
			b.label = label;
			b.id = id;
			m_buttons.push_back(b);
			btnY += btnH;
		};

		addButton("New Project", wxID_NEW);
		addButton("Open Project", wxID_OPEN);
		addButton("Preferences", wxID_PREFERENCES);

		m_fileRects.clear();
		int startY = FromDIP(80);
		int cardH = FromDIP(70);
		int gap = FromDIP(10);
		int cardW = w - m_sidebarWidth - FromDIP(60);

		for (size_t i = 0; i < m_recentFiles.size(); ++i) {
			m_fileRects.push_back(wxRect(m_sidebarWidth + FromDIP(30), startY, cardW, cardH));
			startY += cardH + gap;
		}
		m_contentHeight = startY + FromDIP(50);
		UpdateScrollbar(m_contentHeight);

		int checkW = FromDIP(200);
		int checkH = FromDIP(30);
		m_startupRect = wxRect(m_sidebarWidth + FromDIP(30), h - FromDIP(40), checkW, checkH);
	}

	void DrawSidebarButton(NVGcontext* vg, const ButtonDef& btn, bool hover) {
		if (hover) {
			nvgBeginPath(vg);
			nvgRect(vg, btn.rect.x, btn.rect.y, btn.rect.width, btn.rect.height);
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 20));
			nvgFill(vg);

			nvgBeginPath(vg);
			nvgRect(vg, btn.rect.x, btn.rect.y, 4, btn.rect.height);
			nvgFillColor(vg, nvgRGBA(100, 180, 255, 255));
			nvgFill(vg);
		}

		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, hover ? nvgRGBA(255, 255, 255, 255) : nvgRGBA(200, 200, 210, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, btn.rect.x + 60, btn.rect.y + btn.rect.height / 2, btn.label.ToStdString().c_str(), nullptr);
	}

	void DrawRecentFileCard(NVGcontext* vg, int index, const wxRect& rect) {
		bool hover = (index == m_hoverFile);
		float x = rect.x;
		float y = rect.y;
		float w = rect.width;
		float h = rect.height;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 6.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);

		NVGpaint shadow = nvgBoxGradient(vg, x, y + 2, w, h, 6.0f, 10.0f, nvgRGBA(0, 0, 0, 20), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgRect(vg, x - 10, y - 10, w + 20, h + 20);
		nvgRoundedRect(vg, x, y, w, h, 6.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, shadow);
		nvgFill(vg);

		if (hover) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, x, y, w, h, 6.0f);
			nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		}

		wxFileName fn(m_recentFiles[index]);
		wxString name = fn.GetFullName();
		wxString path = fn.GetPath();

		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans-bold");
		nvgFillColor(vg, nvgRGBA(50, 50, 50, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, x + 20, y + 15, name.ToStdString().c_str(), nullptr);

		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
		nvgText(vg, x + 20, y + 40, path.ToStdString().c_str(), nullptr);
	}

	void DrawStartupCheckbox(NVGcontext* vg, const wxRect& rect, bool hover) {
		float boxSz = 16.0f;
		float boxX = rect.x;
		float boxY = rect.y + (rect.height - boxSz) / 2;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, boxX, boxY, boxSz, boxSz, 4.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		bool checked = (g_settings.getInteger(Config::WELCOME_DIALOG) != 0);
		if (checked) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, boxX + 4, boxY + 8);
			nvgLineTo(vg, boxX + 7, boxY + 11);
			nvgLineTo(vg, boxX + 12, boxY + 4);
			nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		}

		nvgFontSize(vg, 14.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(100, 100, 100, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, boxX + 24, boxY + boxSz / 2, "Show Welcome Screen on Startup", nullptr);
	}
};

WelcomeDialog::WelcomeDialog(const wxString& titleText, const wxString& versionText, const wxSize& size, const wxBitmap& rmeLogo, const std::vector<wxString>& recentFiles) :
	wxDialog(nullptr, wxID_ANY, titleText, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
	Centre();

	m_canvas = new WelcomeCanvas(this, rmeLogo, recentFiles);
	Bind(WELCOME_DIALOG_ACTION, &WelcomeDialog::OnCanvasAction, this);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_canvas, 1, wxEXPAND);
	SetSizer(sizer);

	SetSize(FromDIP(800), FromDIP(500));
}

WelcomeDialog::~WelcomeDialog() {
}

void WelcomeDialog::OnCanvasAction(wxCommandEvent& event) {
	int id = event.GetId();
	if (id == wxID_PREFERENCES) {
		PreferencesWindow preferences_window(this, true);
		preferences_window.ShowModal();
	} else if (id == wxID_NEW) {
		wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
		newEvent->SetId(wxID_NEW);
		QueueEvent(newEvent);
	} else if (id == wxID_OPEN) {
		if (event.GetString().IsEmpty()) {
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
		} else {
			wxCommandEvent* newEvent = new wxCommandEvent(WELCOME_DIALOG_ACTION);
			newEvent->SetId(wxID_OPEN);
			newEvent->SetString(event.GetString());
			QueueEvent(newEvent);
		}
	}
}

void WelcomeDialog::OnButtonClicked(wxCommandEvent& event) {}
void WelcomeDialog::OnCheckboxClicked(wxCommandEvent& event) {}
void WelcomeDialog::OnRecentFileClicked(wxCommandEvent& event) {}
