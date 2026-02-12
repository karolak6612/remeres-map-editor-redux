#ifndef RME_UI_DIALOGS_LOADING_DIALOG_H_
#define RME_UI_DIALOGS_LOADING_DIALOG_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <wx/dialog.h>
#include <wx/timer.h>
#include <vector>

class LoadingCanvas : public NanoVGCanvas {
public:
	LoadingCanvas(wxWindow* parent);
	~LoadingCanvas();

	void SetProgress(int percent, const wxString& message);
	void OnTimer(wxTimerEvent& evt);

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;

private:
	int m_percent = 0;
	wxString m_message;
	wxTimer m_timer;
	float m_animAngle = 0.0f;

	struct Particle {
		float x, y, vx, vy, life, maxLife;
	};
	std::vector<Particle> m_particles;
};

class NanoVGLoadingDialog : public wxDialog {
public:
	NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message, bool canCancel = false);
	~NanoVGLoadingDialog();

	void Update(int percent, const wxString& message);
	bool IsCancelled() const { return m_cancelled; }

private:
	void OnCancel(wxCommandEvent& event);

	LoadingCanvas* m_canvas;
	bool m_cancelled = false;
};

#endif
