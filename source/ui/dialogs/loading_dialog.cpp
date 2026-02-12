#include "ui/dialogs/loading_dialog.h"
#include <nanovg.h>
#include "ui/theme.h"
#include <cmath>
#include <algorithm>
#include <format>
#include <cstdlib>

// ============================================================================
// Loading Canvas

LoadingCanvas::LoadingCanvas(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxBORDER_NONE) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_timer.Bind(wxEVT_TIMER, &LoadingCanvas::OnTimer, this);
	m_timer.Start(16); // ~60 FPS
}

LoadingCanvas::~LoadingCanvas() {
	m_timer.Stop();
}

void LoadingCanvas::SetProgress(int percent, const wxString& message) {
	m_percent = percent;
	m_message = message;
	Refresh();
}

void LoadingCanvas::OnTimer(wxTimerEvent& evt) {
	m_animAngle += 0.1f;
	if (m_animAngle > 2 * M_PI) {
		m_animAngle -= 2 * M_PI;
	}

	// Update particles
	for (auto& p : m_particles) {
		p.x += p.vx;
		p.y += p.vy;
		p.life -= 0.05f;
	}

	// Remove dead particles
	m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
		[](const Particle& p) { return p.life <= 0; }), m_particles.end());

	// Add new particles if loading
	if (m_percent < 100) {
		// Spawn particles around the ring
		int w = GetClientSize().GetWidth();
		int h = GetClientSize().GetHeight();
		float cx = w / 2.0f;
		float cy = h / 2.0f;
		float radius = 60.0f;

		// Spawn at current progress position
		float progressAngle = (m_percent / 100.0f) * 2 * M_PI - M_PI / 2;
		float px = cx + cos(progressAngle) * radius;
		float py = cy + sin(progressAngle) * radius;

		Particle p;
		p.x = px + (rand() % 10 - 5);
		p.y = py + (rand() % 10 - 5);
		p.vx = (rand() % 100 - 50) / 100.0f;
		p.vy = (rand() % 100 - 50) / 100.0f;
		p.life = 1.0f;
		p.maxLife = 1.0f;
		m_particles.push_back(p);
	}

	Refresh();
}

void LoadingCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	float cx = width / 2.0f;
	float cy = height / 2.0f;
	float radius = 60.0f;
	float thickness = 8.0f;

	// Background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBA(30, 30, 35, 255));
	nvgFill(vg);

	// Track ring
	nvgBeginPath(vg);
	nvgArc(vg, cx, cy, radius, 0, 2 * M_PI, NVG_CW);
	nvgStrokeColor(vg, nvgRGBA(50, 50, 55, 255));
	nvgStrokeWidth(vg, thickness);
	nvgStroke(vg);

	// Progress ring
	float startAngle = -M_PI / 2;
	float endAngle = startAngle + (m_percent / 100.0f) * 2 * M_PI;

	if (m_percent > 0) {
		nvgBeginPath(vg);
		nvgArc(vg, cx, cy, radius, startAngle, endAngle, NVG_CW);
		wxColour acc = Theme::Get(Theme::Role::Accent);
		nvgStrokeColor(vg, nvgRGBA(acc.Red(), acc.Green(), acc.Blue(), 255));
		nvgStrokeWidth(vg, thickness);
		nvgStroke(vg);

		// Glow at the tip
		float tipX = cx + cos(endAngle) * radius;
		float tipY = cy + sin(endAngle) * radius;

		NVGpaint glow = nvgRadialGradient(vg, tipX, tipY, 0, 20, nvgRGBA(acc.Red(), acc.Green(), acc.Blue(), 200), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgCircle(vg, tipX, tipY, 20);
		nvgFillPaint(vg, glow);
		nvgFill(vg);
	}

	// Particles
	for (const auto& p : m_particles) {
		float alpha = p.life / p.maxLife;
		nvgBeginPath(vg);
		nvgCircle(vg, p.x, p.y, 2.0f);
		wxColour acc = Theme::Get(Theme::Role::Accent);
		nvgFillColor(vg, nvgRGBAf(acc.Red() / 255.0f, acc.Green() / 255.0f, acc.Blue() / 255.0f, alpha));
		nvgFill(vg);
	}

	// Text
	nvgFontSize(vg, 24.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

	std::string pctStr = std::format("{}%", m_percent);
	nvgText(vg, cx, cy, pctStr.c_str(), nullptr);

	nvgFontSize(vg, 16.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgText(vg, cx, cy + radius + 30, m_message.ToStdString().c_str(), nullptr);
}

// ============================================================================
// NanoVGLoadingDialog

NanoVGLoadingDialog::NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message, bool canCancel) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, FromDIP(wxSize(400, 300)), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP) {

	SetBackgroundColour(wxColour(30, 30, 35));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	m_canvas = new LoadingCanvas(this);
	sizer->Add(m_canvas, 1, wxEXPAND);

	if (canCancel) {
		wxButton* btn = new wxButton(this, wxID_CANCEL, "Cancel");
		sizer->Add(btn, 0, wxALIGN_CENTER | wxALL, 10);
		Bind(wxEVT_BUTTON, &NanoVGLoadingDialog::OnCancel, this, wxID_CANCEL);
	}

	SetSizer(sizer);
	Centre();

	Update(0, message);
}

NanoVGLoadingDialog::~NanoVGLoadingDialog() {
}

void NanoVGLoadingDialog::Update(int percent, const wxString& message) {
	m_canvas->SetProgress(percent, message);
}

void NanoVGLoadingDialog::OnCancel(wxCommandEvent& event) {
	m_cancelled = true;
}
