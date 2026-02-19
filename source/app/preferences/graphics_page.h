#ifndef RME_PREFERENCES_GRAPHICS_PAGE_H
#define RME_PREFERENCES_GRAPHICS_PAGE_H

#include "preferences_page.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filepicker.h>
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>

class GraphicsPage : public PreferencesPage {
public:
	GraphicsPage(wxWindow* parent);
	void Apply() override;

private:
	wxCheckBox* hide_items_when_zoomed_chkbox;
	wxCheckBox* icon_selection_shadow_chkbox;
	wxCheckBox* use_memcached_chkbox;
	wxCheckBox* anti_aliasing_chkbox;

	wxChoice* screen_shader_choice;
	wxChoice* icon_background_choice;
	wxChoice* screenshot_format_choice;

	wxColourPickerCtrl* cursor_color_pick;
	wxColourPickerCtrl* cursor_alt_color_pick;

	wxDirPickerCtrl* screenshot_directory_picker;

	wxSpinCtrl* fps_limit_spin;
	wxCheckBox* show_fps_chkbox;
};

#endif
