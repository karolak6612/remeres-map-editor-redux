#ifndef RME_PREFERENCES_INTERFACE_PAGE_H
#define RME_PREFERENCES_INTERFACE_PAGE_H

#include "preferences_page.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/slider.h>

class InterfacePage : public PreferencesPage {
public:
	InterfacePage(wxWindow* parent);
	void Apply() override;

private:
	wxChoice* AddPaletteStyleChoice(wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting);
	bool SetPaletteStyleChoice(wxChoice* ctrl, int key);

	wxChoice* terrain_palette_style_choice;
	wxChoice* collection_palette_style_choice;
	wxChoice* doodad_palette_style_choice;
	wxChoice* item_palette_style_choice;
	wxChoice* raw_palette_style_choice;

	wxChoice* theme_choice;

	wxCheckBox* large_terrain_tools_chkbox;
	wxCheckBox* large_collection_tools_chkbox;
	wxCheckBox* large_doodad_sizebar_chkbox;
	wxCheckBox* large_item_sizebar_chkbox;
	wxCheckBox* large_house_sizebar_chkbox;
	wxCheckBox* large_raw_sizebar_chkbox;
	wxCheckBox* large_container_icons_chkbox;
	wxCheckBox* large_pick_item_icons_chkbox;

	wxCheckBox* switch_mousebtn_chkbox;
	wxCheckBox* doubleclick_properties_chkbox;
	wxCheckBox* inversed_scroll_chkbox;

	wxSlider* scroll_speed_slider;
	wxSlider* zoom_speed_slider;
};

#endif
