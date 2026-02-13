#include "ui/dialogs/nanovg_loading_dialog.h"
#include "app/main.h"
#include "ui/theme.h"
#include <glad/glad.h>
#include <nanovg.h>
#include <wx/sizer.h>
#include <random>
#include <cmath>
#include <format>

static std::mt19937 g_rng;

NanoVGLoadingDialog::NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message, bool canCancel) :
    wxDialog(parent, wxID_ANY, title, wxDefaultPosition, FromDIP(wxSize(400, 300)), wxSTAY_ON_TOP | wxBORDER_NONE | wxFRAME_SHAPED),
    m_canCancel(canCancel)
{
    SetBackgroundColour(*wxBLACK);

    wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
    m_canvas = newd LoadingCanvas(this, canCancel);
    m_canvas->SetMessage(message);
    sizer->Add(m_canvas, 1, wxEXPAND);
    SetSizer(sizer);

    CenterOnParent();

    Bind(wxEVT_CLOSE_WINDOW, &NanoVGLoadingDialog::OnClose, this);
}

NanoVGLoadingDialog::~NanoVGLoadingDialog() {
}

void NanoVGLoadingDialog::UpdateProgress(int percent, const wxString& msg) {
    m_canvas->SetProgress(percent);
    if (!msg.IsEmpty()) {
        m_canvas->SetMessage(msg);
    }
}

void NanoVGLoadingDialog::OnClose(wxCloseEvent& evt) {
    if (m_canCancel) {
        m_cancelled = true;
        m_canvas->SetCancelled();
    }
    // Don't destroy immediately if controlled by LoadingManager
}

// ----------------------------------------------------------------------------
// LoadingCanvas
// ----------------------------------------------------------------------------

LoadingCanvas::LoadingCanvas(wxWindow* parent, bool canCancel) :
    NanoVGCanvas(parent, wxID_ANY, wxWANTS_CHARS),
    m_canCancel(canCancel)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_timer.SetOwner(this);
    m_timer.Start(16); // ~60fps

    Bind(wxEVT_TIMER, &LoadingCanvas::OnTimer, this);
    Bind(wxEVT_LEFT_DOWN, &LoadingCanvas::OnMouse, this);
    Bind(wxEVT_MOTION, &LoadingCanvas::OnMouse, this);
}

LoadingCanvas::~LoadingCanvas() {
    m_timer.Stop();
}

void LoadingCanvas::SetMessage(const wxString& msg) {
    m_message = msg;
    Refresh();
}

void LoadingCanvas::SetProgress(int percent) {
    if (m_progress != percent) {
        m_progress = percent;

        // Spawn particles on progress change
        int newParticles = 5;
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        int w, h;
        GetClientSize(&w, &h);
        float cx = w * 0.5f;
        float cy = h * 0.4f;

        for (int i=0; i<newParticles; ++i) {
            Particle p;
            float angle = dist(g_rng) * 3.14159f;
            float speed = 2.0f + std::abs(dist(g_rng)) * 2.0f;
            float radius = 60.0f;

            p.x = cx + cos(angle) * radius;
            p.y = cy + sin(angle) * radius;
            p.vx = cos(angle) * speed;
            p.vy = sin(angle) * speed;
            p.life = 1.0f;
            p.maxLife = 1.0f + std::abs(dist(g_rng)) * 0.5f;
            p.size = 2.0f + std::abs(dist(g_rng)) * 2.0f;
            m_particles.push_back(p);
        }

        Refresh();
    }
}

void LoadingCanvas::UpdateParticles(int width, int height) {
    for (auto& p : m_particles) {
        p.x += p.vx;
        p.y += p.vy;
        p.life -= 0.02f;
        p.size *= 0.95f;
    }

    // Remove dead particles
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) {
        return p.life <= 0.0f;
    }), m_particles.end());
}

void LoadingCanvas::OnTimer(wxTimerEvent& evt) {
    m_spinAngle += 0.05f;
    if (m_spinAngle > 3.14159f * 2.0f) {
        m_spinAngle -= 3.14159f * 2.0f;
    }

    int w, h;
    GetClientSize(&w, &h);
    UpdateParticles(w, h);
    Refresh();
}

void LoadingCanvas::OnMouse(wxMouseEvent& evt) {
    if (evt.Dragging() || evt.Moving()) {
        if (m_canCancel) {
            bool hover = m_cancelBtnRect.Contains(evt.GetPosition());
            if (hover != m_cancelHover) {
                m_cancelHover = hover;
                Refresh();
            }
            SetCursor(hover ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));
        }
    } else if (evt.LeftDown()) {
        if (m_canCancel && m_cancelBtnRect.Contains(evt.GetPosition())) {
            // Find parent dialog
            NanoVGLoadingDialog* dlg = dynamic_cast<NanoVGLoadingDialog*>(GetParent());
            if (dlg) {
                wxCloseEvent ce;
                dlg->GetEventHandler()->ProcessEvent(ce);
            }
        }
    }
}

void LoadingCanvas::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
    float cx = width * 0.5f;
    float cy = height * 0.4f;
    float r = 60.0f;

    // Background (Premium Dark)
    NVGpaint bg = nvgRadialGradient(vg, cx, cy, 0, width, nvgRGBA(45, 45, 50, 255), nvgRGBA(20, 20, 25, 255));
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, width, height);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    // Particles
    for (const auto& p : m_particles) {
        nvgBeginPath(vg);
        nvgCircle(vg, p.x, p.y, p.size);
        nvgFillColor(vg, nvgRGBA(0, 120, 215, (int)(255 * p.life)));
        nvgFill(vg);
    }

    // Progress Ring Background
    nvgBeginPath(vg);
    nvgArc(vg, cx, cy, r, 0, 3.14159f * 2.0f, NVG_CW);
    nvgStrokeColor(vg, nvgRGBA(60, 60, 65, 255));
    nvgStrokeWidth(vg, 8.0f);
    nvgStroke(vg);

    // Animated Progress Ring
    float startAngle = -3.14159f * 0.5f + m_spinAngle;
    float sweep = (m_progress / 100.0f) * 3.14159f * 2.0f;
    if (m_progress <= 0) sweep = 0.1f; // Show at least something

    nvgBeginPath(vg);
    nvgArc(vg, cx, cy, r, startAngle, startAngle + sweep, NVG_CW);
    nvgStrokeColor(vg, nvgRGBA(0, 120, 215, 255));
    nvgStrokeWidth(vg, 8.0f);
    nvgStroke(vg);

    // Glow
    nvgSave(vg);
    nvgStrokeWidth(vg, 12.0f);
    nvgStrokeColor(vg, nvgRGBA(0, 120, 215, 60));
    nvgStroke(vg);
    nvgRestore(vg);

    // Percentage Text
    nvgFontSize(vg, 28.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));

    std::string pctStr = std::format("{}%", m_progress);
    nvgText(vg, cx, cy, pctStr.c_str(), nullptr);

    // Message Text
    nvgFontSize(vg, 16.0f);
    nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgTextBox(vg, 40, cy + r + 30, width - 80, m_message.ToStdString().c_str(), nullptr);

    // Cancel Button
    if (m_canCancel) {
        float btnW = 120.0f;
        float btnH = 36.0f;
        float btnX = cx - btnW * 0.5f;
        float btnY = height - btnH - 30.0f;

        m_cancelBtnRect = wxRect((int)btnX, (int)btnY, (int)btnW, (int)btnH);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, btnX, btnY, btnW, btnH, 4.0f);
        if (m_cancelHover) {
            nvgFillColor(vg, nvgRGBA(200, 50, 50, 255));
        } else {
            nvgFillColor(vg, nvgRGBA(60, 60, 65, 255));
        }
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, cx, btnY + btnH * 0.5f, "Cancel", nullptr);
    }
}
