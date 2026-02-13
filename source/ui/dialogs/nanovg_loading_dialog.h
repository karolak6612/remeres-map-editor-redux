#ifndef RME_UI_NANOVG_LOADING_DIALOG_H_
#define RME_UI_NANOVG_LOADING_DIALOG_H_

#include "app/main.h"
#include <wx/dialog.h>
#include <wx/timer.h>
#include <vector>
#include "util/nanovg_canvas.h"

class LoadingCanvas;

class NanoVGLoadingDialog : public wxDialog {
public:
    NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message, bool canCancel);
    ~NanoVGLoadingDialog();

    void UpdateProgress(int percent, const wxString& msg);
    bool IsCancelled() const { return m_cancelled; }

private:
    void OnClose(wxCloseEvent& evt);

    LoadingCanvas* m_canvas;
    bool m_cancelled = false;
    bool m_canCancel;
};

// The canvas that does the drawing
class LoadingCanvas : public NanoVGCanvas {
public:
    LoadingCanvas(wxWindow* parent, bool canCancel);
    ~LoadingCanvas();

    void SetMessage(const wxString& msg);
    void SetProgress(int percent);
    void SetCancelled() { m_isCancelled = true; Refresh(); }
    bool CheckCancelClick(const wxPoint& pt);

    void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

    // Events
    void OnTimer(wxTimerEvent& evt);
    void OnMouse(wxMouseEvent& evt);

private:
    wxString m_message;
    int m_progress = 0;
    bool m_canCancel;
    bool m_isCancelled = false;

    // Cancel button state
    wxRect m_cancelBtnRect;
    bool m_cancelHover = false;

    // Animation state
    wxTimer m_timer;
    float m_spinAngle = 0.0f;

    struct Particle {
        float x, y;
        float vx, vy;
        float life;
        float maxLife;
        float size;
    };
    std::vector<Particle> m_particles;

    void UpdateParticles(int width, int height);
};

#endif
