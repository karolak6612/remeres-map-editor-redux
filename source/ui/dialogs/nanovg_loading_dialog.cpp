#include "ui/dialogs/nanovg_loading_dialog.h"
#include "ui/theme.h"
#include <nanovg.h>
#include <wx/app.h>
#include <cmath>

LoadingCanvas::LoadingCanvas(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxWANTS_CHARS) {
	SetBackgroundColour(wxColour(30, 30, 30));
	m_animTimer.Bind(wxEVT_TIMER, &LoadingCanvas::OnTimer, this);
	m_animTimer.Start(16); // ~60fps

	Bind(wxEVT_LEFT_DOWN, &LoadingCanvas::OnMouse, this);
	Bind(wxEVT_MOTION, &LoadingCanvas::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &LoadingCanvas::OnLeave, this);
}

LoadingCanvas::~LoadingCanvas() {
	m_animTimer.Stop();
}

void LoadingCanvas::OnTimer(wxTimerEvent& evt) {
	m_rotation += 0.05f;
	if (m_rotation > M_PI * 2) {
		m_rotation -= M_PI * 2;
	}
	Refresh();
}

void LoadingCanvas::SetProgress(int percent, const wxString& text) {
	m_percent = percent;
	if (!text.IsEmpty()) {
		m_text = text;
	}
	Refresh();
}

void LoadingCanvas::OnMouse(wxMouseEvent& evt) {
	wxPoint pos = evt.GetPosition();
	bool prevHover = m_hoverCancel;
	m_hoverCancel = m_cancelRect.Contains(pos);

	if (prevHover != m_hoverCancel) {
		Refresh();
	}

	if (evt.LeftDown() && m_hoverCancel) {
		m_cancelled = true;
	}

	SetCursor(m_hoverCancel ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));
}

void LoadingCanvas::OnLeave(wxMouseEvent& evt) {
	if (m_hoverCancel) {
		m_hoverCancel = false;
		Refresh();
	}
}

void LoadingCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBA(30, 30, 35, 255));
	nvgFill(vg);

	float cx = width / 2.0f;
	float cy = height / 2.0f - 20.0f;
	float radius = 50.0f;

	// Progress Arc Background
	nvgBeginPath(vg);
	nvgArc(vg, cx, cy, radius, 0, M_PI * 2, NVG_CW);
	nvgStrokeColor(vg, nvgRGBA(60, 60, 65, 255));
	nvgStrokeWidth(vg, 8.0f);
	nvgStroke(vg);

	// Progress Arc Foreground
	float progressAngle = (m_percent / 100.0f) * M_PI * 2;
	float startAngle = -M_PI / 2;
	float endAngle = startAngle + progressAngle;

	nvgBeginPath(vg);
	nvgArc(vg, cx, cy, radius, startAngle, endAngle, NVG_CW);
	nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
	nvgStrokeWidth(vg, 8.0f);
	nvgStrokeLineCap(vg, NVG_ROUND);
	nvgStroke(vg);

	// Rotating Particles
	nvgSave(vg);
	nvgTranslate(vg, cx, cy);
	nvgRotate(vg, m_rotation);

	nvgBeginPath(vg);
	nvgArc(vg, 0, 0, radius - 15.0f, 0, M_PI * 1.5f, NVG_CW);
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 40));
	nvgStrokeWidth(vg, 2.0f);
	nvgStroke(vg);

	float particleX = cos(m_rotation * 2.0f) * (radius + 20.0f);
	float particleY = sin(m_rotation * 2.0f) * (radius + 20.0f);
	nvgBeginPath(vg);
	nvgCircle(vg, particleX, particleY, 4.0f);
	nvgFillColor(vg, nvgRGBA(100, 200, 255, 128));
	nvgFill(vg);

	nvgRestore(vg);

	// Text
	nvgFontSize(vg, 24.0f);
	nvgFontFace(vg, "sans-bold");
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	wxString percentStr = wxString::Format("%d%%", m_percent);
	nvgText(vg, cx, cy, percentStr.ToStdString().c_str(), nullptr);

	nvgFontSize(vg, 16.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgText(vg, cx, cy + radius + 40.0f, m_text.ToStdString().c_str(), nullptr);

	// Cancel Button
	float btnW = 100.0f;
	float btnH = 30.0f;
	float btnX = cx - btnW / 2.0f;
	float btnY = height - 50.0f;
	m_cancelRect = wxRect(btnX, btnY, btnW, btnH);

	nvgBeginPath(vg);
	nvgRoundedRect(vg, btnX, btnY, btnW, btnH, 4.0f);
	if (m_hoverCancel) {
		nvgFillColor(vg, nvgRGBA(70, 70, 75, 255));
	} else {
		nvgFillColor(vg, nvgRGBA(50, 50, 55, 255));
	}
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRoundedRect(vg, btnX, btnY, btnW, btnH, 4.0f);
	nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 255));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgText(vg, cx, btnY + btnH / 2.0f, "Cancel", nullptr);
}

NanoVGLoadingDialog::NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300), wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR | wxBORDER_NONE) {

	SetBackgroundColour(wxColour(30, 30, 30));
	CenterOnParent();

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	m_canvas = new LoadingCanvas(this);
	m_canvas->SetProgress(0, message);
	sizer->Add(m_canvas, 1, wxEXPAND);
	SetSizer(sizer);
}

NanoVGLoadingDialog::~NanoVGLoadingDialog() {
}

bool NanoVGLoadingDialog::Update(int value, const wxString& newmsg) {
	m_canvas->SetProgress(value, newmsg);
	wxYield();
	return !m_canvas->IsCancelled();
}
