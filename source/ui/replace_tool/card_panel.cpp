#include "ui/replace_tool/card_panel.h"
#include "ui/theme.h"
#include <wx/settings.h>
#include <numbers>
#include <nanovg.h>
#include <memory>

CardPanel::CardPanel(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id) {
	Bind(wxEVT_SIZE, &CardPanel::OnSize, this);

	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->AddSpacer(HEADER_HEIGHT);

	m_contentSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->Add(m_contentSizer, 1, wxEXPAND);

	m_footerSizer = new wxBoxSizer(wxVERTICAL);
	m_mainSizer->Add(m_footerSizer, 0, wxEXPAND);

	SetSizer(m_mainSizer);
}

void CardPanel::SetShowFooter(bool show) {
	m_showFooter = show;
	if (m_showFooter) {
		m_mainSizer->SetItemMinSize(m_footerSizer, wxSize(-1, FOOTER_HEIGHT));
	} else {
		m_mainSizer->SetItemMinSize(m_footerSizer, wxSize(-1, 0));
	}
	Layout();
	Refresh();
}

void CardPanel::SetTitle(const wxString& title) {
	m_title = title;
	m_cachedTitleStr.clear();
	Refresh();
}

void CardPanel::OnSize(wxSizeEvent& event) {
	// NanoVGCanvas usually handles resizing GL context
	Refresh();
	event.Skip();
}

void CardPanel::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Determine Colors
	wxColour base = Theme::Get(Theme::Role::Surface);
	wxColour cardBgStart = Theme::Get(Theme::Role::Surface);
	wxColour cardBgEnd = Theme::Get(Theme::Role::Background);
	wxColour borderColor = Theme::Get(Theme::Role::Border);
	wxColour headerBg = Theme::Get(Theme::Role::Header);
	wxColour titleColor = Theme::Get(Theme::Role::Text);

	// Clear background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBA(base.Red(), base.Green(), base.Blue(), base.Alpha()));
	nvgFill(vg);

	// Padding for "shadow"
	float margin = 2.0f;
	float r = 6.0f; // Radius

	// Draw Shadow (simulated with offset rect)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, margin + 2, margin + 2, width - 2 * margin, height - 2 * margin, r);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 40));
	nvgFill(vg);

	// Draw Card Body
	float cardX = margin;
	float cardY = margin;
	float cardW = width - 2 * margin;
	float cardH = height - 2 * margin;

	// Gradient
	NVGpaint paint = nvgLinearGradient(vg, cardX, cardY, cardX, cardY + cardH, nvgRGBA(cardBgStart.Red(), cardBgStart.Green(), cardBgStart.Blue(), cardBgStart.Alpha()), nvgRGBA(cardBgEnd.Red(), cardBgEnd.Green(), cardBgEnd.Blue(), cardBgEnd.Alpha()));

	nvgBeginPath(vg);
	nvgRoundedRect(vg, cardX, cardY, cardW, cardH, r);
	nvgFillPaint(vg, paint);
	nvgFill(vg);

	// Border
	nvgStrokeColor(vg, nvgRGBA(borderColor.Red(), borderColor.Green(), borderColor.Blue(), borderColor.Alpha()));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	// Draw Header if Title exists
	if (!m_title.IsEmpty()) {
		float headerH = (float)HEADER_HEIGHT;
		float x = margin;
		float y = margin;
		float cw = cardW;
		float ch = headerH; // Height of header part

		nvgBeginPath(vg);
		// Start at Bottom-Left of header
		nvgMoveTo(vg, x, y + ch);
		// Left vertical up to start of round
		nvgLineTo(vg, x, y + r);
		// Top-Left Corner
		nvgArc(vg, x + r, y + r, r, NVG_PI, 1.5f * NVG_PI, NVG_CW);
		// Top Line
		nvgLineTo(vg, x + cw - r, y);
		// Top-Right Corner
		nvgArc(vg, x + cw - r, y + r, r, 1.5f * NVG_PI, 0, NVG_CW);
		// Bottom-Right of header part
		nvgLineTo(vg, x + cw, y + ch);
		// Bottom Line (Separator)
		nvgLineTo(vg, x, y + ch);
		nvgClosePath(vg);

		// Fill Header
		nvgFillColor(vg, nvgRGBA(headerBg.Red(), headerBg.Green(), headerBg.Blue(), headerBg.Alpha()));
		nvgFill(vg);

		// Draw Separator Line
		nvgBeginPath(vg);
		nvgMoveTo(vg, x, y + ch);
		nvgLineTo(vg, x + cw, y + ch);
		nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 50));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Draw Text
		static const float TITLE_FONT_SIZE_PX = 12.0f; // 9pt * 4/3 = 12px
		nvgFontSize(vg, TITLE_FONT_SIZE_PX);
		if (nvgFindFont(vg, "sans") != -1) {
			nvgFontFace(vg, "sans");
		}
		nvgFillColor(vg, nvgRGBA(titleColor.Red(), titleColor.Green(), titleColor.Blue(), titleColor.Alpha()));
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		float tx = x + cw / 2.0f;
		float ty = y + ch / 2.0f;

		if (m_cachedTitleStr.empty() && !m_title.IsEmpty()) {
			m_cachedTitleStr = m_title.ToStdString();
		}
		nvgText(vg, tx, ty, m_cachedTitleStr.c_str(), nullptr);
	}

	// Draw Footer if requested
	if (m_showFooter) {
		float footerH = (float)FOOTER_HEIGHT;
		float x = margin;
		float y = height - margin - footerH;
		float cw = cardW;
		float ch = footerH;

		nvgBeginPath(vg);
		// Start at Top-Left of footer
		nvgMoveTo(vg, x, y);
		// Top Line (Separator)
		nvgLineTo(vg, x + cw, y);
		// Right vertical down to start of round
		nvgLineTo(vg, x + cw, y + ch - r);
		// Bottom-Right Corner
		nvgArc(vg, x + cw - r, y + ch - r, r, 0, 0.5f * NVG_PI, NVG_CW);
		// Bottom Line
		nvgLineTo(vg, x + r, y + ch);
		// Bottom-Left Corner
		nvgArc(vg, x + r, y + ch - r, r, 0.5f * NVG_PI, NVG_PI, NVG_CW);
		// Left vertical up
		nvgLineTo(vg, x, y);
		nvgClosePath(vg);

		// Fill Footer
		nvgFillColor(vg, nvgRGBA(headerBg.Red(), headerBg.Green(), headerBg.Blue(), headerBg.Alpha()));
		nvgFill(vg);

		// Draw Separator Line
		nvgBeginPath(vg);
		nvgMoveTo(vg, x, y);
		nvgLineTo(vg, x + cw, y);
		nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 50));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}
}
