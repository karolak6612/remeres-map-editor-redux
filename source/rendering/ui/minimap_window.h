//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_MINIMAP_WINDOW_H_
#define RME_MINIMAP_WINDOW_H_

#include <wx/glcanvas.h>
#include <wx/timer.h>
#include <memory>

#include "rendering/core/graphics.h"

class MinimapDrawer;
class MinimapWindow : public wxGLCanvas {
public:
	MinimapWindow(wxWindow* parent);
	~MinimapWindow() override;

	void OnPaint(wxPaintEvent&);
	void OnEraseBackground(wxEraseEvent&) { }
	void OnMouseClick(wxMouseEvent&);
	void OnSize(wxSizeEvent&);
	void OnClose(wxCloseEvent&);

	void DelayedUpdate();
	void OnDelayedUpdate(wxTimerEvent& event);
	void OnAnimationTimer(wxTimerEvent& event);
	void OnKey(wxKeyEvent& event);

protected:
	std::unique_ptr<MinimapDrawer> drawer;
	wxTimer update_timer;

	// Smooth Panning
	wxTimer animation_timer;
	double anim_current_x = 0.0;
	double anim_current_y = 0.0;
	double anim_target_x = 0.0;
	double anim_target_y = 0.0;
	bool is_animating = false;

	std::unique_ptr<wxGLContext> context;
	std::unique_ptr<NVGcontext, NVGDeleter> nvg;
};

#endif
