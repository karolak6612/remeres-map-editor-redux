//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_TOOLBAR_LIGHT_TOOLBAR_H_
#define RME_UI_TOOLBAR_LIGHT_TOOLBAR_H_

#include <wx/wx.h>
#include <wx/aui/auibar.h>
#include <wx/button.h>
#include <wx/slider.h>

class LightToolBar : public wxEvtHandler {
public:
	LightToolBar(wxWindow* parent);
	~LightToolBar();

	wxAuiToolBar* GetToolbar() const {
		return toolbar;
	}

	void OnServerLightIntensitySlider(wxCommandEvent& event);
	void OnClientBrightnessSlider(wxCommandEvent& event);
	void OnChooseServerLightColor(wxCommandEvent& event);
	void OnToggleLight(wxCommandEvent& event);

	static const wxString PANE_NAME;

private:
	void UpdateServerLightColorButton();

	wxAuiToolBar* toolbar;
	wxSlider* server_light_intensity_slider;
	wxSlider* client_brightness_slider;
	wxButton* server_light_color_button;
};

#endif // RME_UI_TOOLBAR_LIGHT_TOOLBAR_H_
