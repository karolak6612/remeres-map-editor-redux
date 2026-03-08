#ifndef RME_PREFERENCES_INTERFACE_PAGE_H
#define RME_PREFERENCES_INTERFACE_PAGE_H

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/stattext.h>

#include "preferences_page.h"

class InterfacePage : public ScrollablePreferencesPage {
public:
	explicit InterfacePage(wxWindow* parent);
	void Apply() override;

private:
	void UpdateSpeedLabels();
	wxChoice* AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting);
	bool SetPaletteStyleChoice(wxChoice* ctrl, int key);

	wxChoice* terrain_palette_style_choice = nullptr;
	wxChoice* collection_palette_style_choice = nullptr;
	wxChoice* doodad_palette_style_choice = nullptr;
	wxChoice* item_palette_style_choice = nullptr;
	wxChoice* raw_palette_style_choice = nullptr;

	wxChoice* theme_choice = nullptr;

	wxCheckBox* large_terrain_tools_chkbox = nullptr;
	wxCheckBox* large_collection_tools_chkbox = nullptr;
	wxCheckBox* large_doodad_sizebar_chkbox = nullptr;
	wxCheckBox* large_item_sizebar_chkbox = nullptr;
	wxCheckBox* large_house_sizebar_chkbox = nullptr;
	wxCheckBox* large_raw_sizebar_chkbox = nullptr;
	wxCheckBox* large_container_icons_chkbox = nullptr;
	wxCheckBox* large_pick_item_icons_chkbox = nullptr;

	wxCheckBox* switch_mousebtn_chkbox = nullptr;
	wxCheckBox* doubleclick_properties_chkbox = nullptr;
	wxCheckBox* inversed_scroll_chkbox = nullptr;

	wxSlider* scroll_speed_slider = nullptr;
	wxSlider* zoom_speed_slider = nullptr;
	wxStaticText* scroll_speed_value_label = nullptr;
	wxStaticText* zoom_speed_value_label = nullptr;
};

#endif
