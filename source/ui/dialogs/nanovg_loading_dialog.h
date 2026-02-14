#ifndef RME_UI_DIALOGS_NANOVG_LOADING_DIALOG_H_
#define RME_UI_DIALOGS_NANOVG_LOADING_DIALOG_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <wx/dialog.h>
#include <wx/timer.h>

class LoadingCanvas : public NanoVGCanvas {
public:
	LoadingCanvas(wxWindow* parent);
	~LoadingCanvas() override;

	void SetProgress(int percent, const wxString& text);
	bool IsCancelled() const { return m_cancelled; }

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	void OnTimer(wxTimerEvent& evt);
	void OnMouse(wxMouseEvent& evt);
	void OnLeave(wxMouseEvent& evt);

private:
	int m_percent = 0;
	wxString m_text;
	float m_rotation = 0.0f;
	wxTimer m_animTimer;

	// Cancel Button
	wxRect m_cancelRect;
	bool m_hoverCancel = false;
	bool m_cancelled = false;
};

class NanoVGLoadingDialog : public wxDialog {
public:
	NanoVGLoadingDialog(wxWindow* parent, const wxString& title, const wxString& message);
	~NanoVGLoadingDialog() override;

	bool Update(int value, const wxString& newmsg = wxEmptyString);

private:
	LoadingCanvas* m_canvas;
};

#endif
