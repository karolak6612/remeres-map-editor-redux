#include "ui/welcome_canvas.h"
#include "ui/gui.h"
#include "util/image_manager.h"
#include "app/settings.h"
#include "ui/welcome_dialog.h"
#include <nanovg.h>

WelcomeCanvas::WelcomeCanvas(wxWindow* parent, const wxBitmap& logo, const std::vector<wxString>& recentFiles) :
	NanoVGCanvas(parent, wxID_ANY, wxWANTS_CHARS),
	m_logoBitmap(logo),
	m_logoImageId(0),
	m_iconNew(0),
	m_iconOpen(0),
	m_iconPreferences(0),
	m_recentFiles(recentFiles),
	hover_element_index(-1) {

	Bind(wxEVT_LEFT_DOWN, &WelcomeCanvas::OnMouseDown, this);
	Bind(wxEVT_MOTION, &WelcomeCanvas::OnMouseMove, this);
	Bind(wxEVT_SIZE, &WelcomeCanvas::OnSize, this);
	Bind(wxEVT_LEAVE_WINDOW, &WelcomeCanvas::OnLeave, this);

	col_sidebar_bg = nvgRGBA(45, 45, 50, 255);
	col_content_bg = nvgRGBA(30, 30, 35, 255);
	col_text_main = nvgRGBA(230, 230, 230, 255);
	col_text_sub = nvgRGBA(150, 150, 150, 255);
	col_hover_bg = nvgRGBA(60, 60, 65, 255);
	col_button_text = nvgRGBA(240, 240, 240, 255);
}

WelcomeCanvas::~WelcomeCanvas() {
	NVGcontext* vg = GetNVGContext();
	if (vg) {
		if (m_logoImageId > 0) nvgDeleteImage(vg, m_logoImageId);
		if (m_iconNew > 0) nvgDeleteImage(vg, m_iconNew);
		if (m_iconOpen > 0) nvgDeleteImage(vg, m_iconOpen);
		if (m_iconPreferences > 0) nvgDeleteImage(vg, m_iconPreferences);
	}
}

int WelcomeCanvas::CreateBitmapTexture(NVGcontext* vg, const wxBitmap& bmp) {
	if (!bmp.IsOk()) return 0;
	wxImage img = bmp.ConvertToImage();
	if (!img.IsOk()) return 0;

	int w = img.GetWidth();
	int h = img.GetHeight();
	std::vector<uint8_t> rgba(w * h * 4);

	const uint8_t* data = img.GetData();
	const uint8_t* alpha = img.GetAlpha();

	for (int i = 0; i < w * h; ++i) {
		rgba[i * 4 + 0] = data[i * 3 + 0];
		rgba[i * 4 + 1] = data[i * 3 + 1];
		rgba[i * 4 + 2] = data[i * 3 + 2];
		if (alpha) rgba[i * 4 + 3] = alpha[i];
		else rgba[i * 4 + 3] = 255;
	}

	return nvgCreateImageRGBA(vg, w, h, 0, rgba.data());
}

int WelcomeCanvas::CreateIconTexture(NVGcontext* vg, const wxString& iconName) {
	wxBitmap bmp = IMAGE_MANAGER.GetBitmap(iconName.ToStdString().c_str(), wxSize(16, 16));
	return CreateBitmapTexture(vg, bmp);
}

void WelcomeCanvas::UpdateLayout() {
	elements.clear();
	wxSize size = GetClientSize();
	int sidebarWidth = size.x * 0.35;
	if (sidebarWidth < 200) sidebarWidth = 200;
	if (sidebarWidth > 300) sidebarWidth = 300;

	int contentWidth = size.x - sidebarWidth;

	// --- Sidebar ---
	int y = 200; // Start below logo
	int btnHeight = 40;
	int btnPadding = 10;
	int btnWidth = sidebarWidth - 40;
	int btnX = 20;

	elements.push_back({wxRect(btnX, y, btnWidth, btnHeight), wxID_NEW, "New Project", "", 0, false, false, false});
	y += btnHeight + btnPadding;
	elements.push_back({wxRect(btnX, y, btnWidth, btnHeight), wxID_OPEN, "Open Project", "", 0, false, false, false});
	y += btnHeight + btnPadding;
	elements.push_back({wxRect(btnX, y, btnWidth, btnHeight), wxID_PREFERENCES, "Preferences", "", 0, false, false, false});

	// --- Content ---
	int contentX = sidebarWidth + 20;
	int contentY = 60; // Below title
	int itemHeight = 50;
	int itemWidth = contentWidth - 40;

	for (size_t i = 0; i < m_recentFiles.size(); ++i) {
		wxFileName fn(m_recentFiles[i]);
		elements.push_back({
			wxRect(contentX, contentY, itemWidth, itemHeight),
			(int)i, // ID is index
			fn.GetFullName().ToStdString(),
			fn.GetPath().ToStdString(),
			0,
			true,
			false,
			false
		});
		contentY += itemHeight + 2;
	}

	// --- Footer Checkbox ---
	int footerY = size.y - 40;
	elements.push_back({
		wxRect(contentX, footerY, 200, 20),
		wxID_ANY,
		"Show on Startup",
		"",
		0,
		false,
		true,
		g_settings.getInteger(Config::WELCOME_DIALOG) != 0
	});
}

int WelcomeCanvas::GetHoveredElement(int x, int y) const {
	for (size_t i = 0; i < elements.size(); ++i) {
		if (elements[i].rect.Contains(x, y)) {
			return (int)i;
		}
	}
	return -1;
}

void WelcomeCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	if (elements.empty()) UpdateLayout();

	// Ensure textures
	if (m_logoImageId == 0) m_logoImageId = CreateBitmapTexture(vg, m_logoBitmap);
	if (m_iconNew == 0) m_iconNew = CreateIconTexture(vg, ICON_NEW);
	if (m_iconOpen == 0) m_iconOpen = CreateIconTexture(vg, ICON_OPEN);
	if (m_iconPreferences == 0) m_iconPreferences = CreateIconTexture(vg, ICON_GEAR);

	// Update icon IDs in elements (hacky but works)
	for (auto& el : elements) {
		if (el.id == wxID_NEW) el.iconId = m_iconNew;
		else if (el.id == wxID_OPEN) el.iconId = m_iconOpen;
		else if (el.id == wxID_PREFERENCES) el.iconId = m_iconPreferences;
	}

	int sidebarWidth = width * 0.35;
	if (sidebarWidth < 200) sidebarWidth = 200;
	if (sidebarWidth > 300) sidebarWidth = 300;

	// Backgrounds
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, sidebarWidth, height);
	nvgFillColor(vg, col_sidebar_bg);
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, sidebarWidth, 0, width - sidebarWidth, height);
	nvgFillColor(vg, col_content_bg);
	nvgFill(vg);

	// Logo
	if (m_logoImageId > 0) {
		int lw, lh;
		nvgImageSize(vg, m_logoImageId, &lw, &lh);
		float scale = 1.0f;
		if (lw > sidebarWidth - 40) scale = (float)(sidebarWidth - 40) / lw;
		float dw = lw * scale;
		float dh = lh * scale;
		float dx = (sidebarWidth - dw) / 2;
		float dy = 40;

		NVGpaint imgPaint = nvgImagePattern(vg, dx, dy, dw, dh, 0, m_logoImageId, 1.0f);
		nvgBeginPath(vg);
		nvgRect(vg, dx, dy, dw, dh);
		nvgFillPaint(vg, imgPaint);
		nvgFill(vg);

		// Version text
		nvgFontSize(vg, 12.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
		nvgFillColor(vg, col_text_sub);

		char verBuf[64];
		snprintf(verBuf, 64, "v%s", wxString(__W_RME_VERSION__).mb_str().data());
		nvgText(vg, sidebarWidth / 2.0f, dy + dh + 10, verBuf, nullptr);
	}

	// Content Title
	nvgFontSize(vg, 24.0f);
	nvgFontFace(vg, "sans-bold");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgFillColor(vg, col_text_main);
	nvgText(vg, sidebarWidth + 20, 20, "Recent Projects", nullptr);

	// Elements
	for (size_t i = 0; i < elements.size(); ++i) {
		const auto& el = elements[i];
		bool hover = ((int)i == hover_element_index);

		if (el.is_checkbox) {
			// Checkbox rendering
			nvgFontSize(vg, 14.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, col_text_sub);
			nvgText(vg, el.rect.x + 24, el.rect.y + el.rect.height/2, el.label.c_str(), nullptr);

			// Box
			nvgBeginPath(vg);
			nvgRoundedRect(vg, el.rect.x, el.rect.y + 2, 16, 16, 3);
			nvgStrokeColor(vg, col_text_sub);
			nvgStroke(vg);

			if (el.checked) {
				nvgBeginPath(vg);
				nvgRect(vg, el.rect.x + 4, el.rect.y + 6, 8, 8);
				nvgFillColor(vg, col_text_main);
				nvgFill(vg);
			}
			continue;
		}

		// Background for buttons/items
		if (hover) {
			nvgBeginPath(vg);
			nvgRoundedRect(vg, el.rect.x, el.rect.y, el.rect.width, el.rect.height, 4.0f);
			nvgFillColor(vg, col_hover_bg);
			nvgFill(vg);
		}

		if (el.is_recent_file) {
			nvgFontSize(vg, 16.0f);
			nvgFontFace(vg, "sans-bold");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgFillColor(vg, col_text_main);
			nvgText(vg, el.rect.x + 10, el.rect.y + 8, el.label.c_str(), nullptr);

			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgFillColor(vg, col_text_sub);
			nvgText(vg, el.rect.x + 10, el.rect.y + 30, el.sublabel.c_str(), nullptr);
		} else {
			// Button
			// Icon
			if (el.iconId > 0) {
				NVGpaint imgPaint = nvgImagePattern(vg, el.rect.x + 10, el.rect.y + 12, 16, 16, 0, el.iconId, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, el.rect.x + 10, el.rect.y + 12, 16, 16);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}

			nvgFontSize(vg, 14.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, col_button_text);
			nvgText(vg, el.rect.x + 36, el.rect.y + el.rect.height/2, el.label.c_str(), nullptr);
		}
	}
}

wxSize WelcomeCanvas::DoGetBestClientSize() const {
	return FromDIP(wxSize(800, 500));
}

void WelcomeCanvas::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
}

void WelcomeCanvas::OnMouseMove(wxMouseEvent& event) {
	int index = GetHoveredElement(event.GetX(), event.GetY());
	if (index != hover_element_index) {
		hover_element_index = index;
		Refresh();
		SetCursor(index != -1 ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
	}
	event.Skip();
}

void WelcomeCanvas::OnLeave(wxMouseEvent& event) {
	if (hover_element_index != -1) {
		hover_element_index = -1;
		Refresh();
		SetCursor(wxNullCursor);
	}
	event.Skip();
}

void WelcomeCanvas::OnMouseDown(wxMouseEvent& event) {
	int index = GetHoveredElement(event.GetX(), event.GetY());
	if (index != -1) {
		const auto& el = elements[index];
		if (el.is_checkbox) {
			// Toggle setting
			bool newVal = !el.checked;
			g_settings.setInteger(Config::WELCOME_DIALOG, newVal ? 1 : 0);
			UpdateLayout(); // Rebuild elements to update state
			Refresh();
		} else if (el.is_recent_file) {
			wxCommandEvent newEvent(WELCOME_DIALOG_ACTION);
			newEvent.SetId(wxID_OPEN);
			newEvent.SetString(m_recentFiles[el.id]); // id is index
			GetEventHandler()->ProcessEvent(newEvent);
		} else {
			// Button
			wxCommandEvent newEvent(WELCOME_DIALOG_ACTION);
			newEvent.SetId(el.id);
			GetEventHandler()->ProcessEvent(newEvent);
		}
	}
	event.Skip();
}
