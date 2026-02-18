#include "app/preferences/interface_page.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"

InterfacePage::InterfacePage(wxWindow* parent) : PreferencesPage(parent) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);
	terrain_palette_style_choice = AddPaletteStyleChoice(
		subsizer,
		"Terrain Palette Style:",
		"Configures the look of the terrain palette.",
		g_settings.getString(Config::PALETTE_TERRAIN_STYLE)
	);
	collection_palette_style_choice = AddPaletteStyleChoice(
		subsizer,
		"Collections Palette Style:",
		"Configures the look of the collections palette.",
		g_settings.getString(Config::PALETTE_COLLECTION_STYLE)
	);
	doodad_palette_style_choice = AddPaletteStyleChoice(
		subsizer,
		"Doodad Palette Style:",
		"Configures the look of the doodad palette.",
		g_settings.getString(Config::PALETTE_DOODAD_STYLE)
	);
	item_palette_style_choice = AddPaletteStyleChoice(
		subsizer,
		"Item Palette Style:",
		"Configures the look of the item palette.",
		g_settings.getString(Config::PALETTE_ITEM_STYLE)
	);
	raw_palette_style_choice = AddPaletteStyleChoice(
		subsizer,
		"RAW Palette Style:",
		"Configures the look of the raw palette.",
		g_settings.getString(Config::PALETTE_RAW_STYLE)
	);

	sizer->Add(subsizer, 0, wxALL, 6);

	sizer->AddSpacer(10);

	large_terrain_tools_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large terrain palette tool && size icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	sizer->Add(large_terrain_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_collection_tools_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large collections palette tool && size icons");
	large_collection_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	sizer->Add(large_collection_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_doodad_sizebar_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large doodad size palette icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	sizer->Add(large_doodad_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_item_sizebar_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large item size palette icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	sizer->Add(large_item_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_house_sizebar_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large house palette size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	sizer->Add(large_house_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_raw_sizebar_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large raw palette size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	sizer->Add(large_raw_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_container_icons_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	sizer->Add(large_container_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	large_pick_item_icons_chkbox = newd wxCheckBox(this, wxID_ANY, "Use large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	sizer->Add(large_pick_item_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	switch_mousebtn_chkbox = newd wxCheckBox(this, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 5);

	doubleclick_properties_chkbox = newd wxCheckBox(this, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 5);

	inversed_scroll_chkbox = newd wxCheckBox(this, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When this checkbox is checked, dragging the map using the center mouse button will be inversed (default RTS behaviour).");
	sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(this, wxID_ANY, "Scroll speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_scrollspeed = int(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 10);
	scroll_speed_slider = newd wxSlider(this, wxID_ANY, true_scrollspeed, 1, std::max(true_scrollspeed, 100));
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	sizer->Add(newd wxStaticText(this, wxID_ANY, "Zoom speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_zoomspeed = int(g_settings.getFloat(Config::ZOOM_SPEED) * 10);
	zoom_speed_slider = newd wxSlider(this, wxID_ANY, true_zoomspeed, 1, std::max(true_zoomspeed, 100));
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	SetSizerAndFit(sizer);
}

wxChoice* InterfacePage::AddPaletteStyleChoice(wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting) {
	wxStaticText* text;
	sizer->Add(text = newd wxStaticText(this, wxID_ANY, short_description), 0);

	wxChoice* choice = newd wxChoice(this, wxID_ANY);
	sizer->Add(choice, 0);

	choice->Append("Large Icons");
	choice->Append("Small Icons");
	choice->Append("Listbox with Icons");

	text->SetToolTip(description);
	choice->SetToolTip(description);

	if (setting == "large icons") {
		choice->SetSelection(0);
	} else if (setting == "small icons") {
		choice->SetSelection(1);
	} else if (setting == "listbox") {
		choice->SetSelection(2);
	}

	return choice;
}

bool InterfacePage::SetPaletteStyleChoice(wxChoice* ctrl, int key) {
	std::string current = g_settings.getString(key);
	std::string new_val = current;

	if (ctrl->GetSelection() == 0) {
		new_val = "large icons";
	} else if (ctrl->GetSelection() == 1) {
		new_val = "small icons";
	} else if (ctrl->GetSelection() == 2) {
		new_val = "listbox";
	}

	if (current != new_val) {
		g_settings.setString(key, new_val);
		return true;
	}
	return false;
}

void InterfacePage::Apply() {
	bool palette_update_needed = false;

	if (SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE)) {
		palette_update_needed = true;
	}
	if (SetPaletteStyleChoice(collection_palette_style_choice, Config::PALETTE_COLLECTION_STYLE)) {
		palette_update_needed = true;
	}
	if (SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE)) {
		palette_update_needed = true;
	}
	if (SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE)) {
		palette_update_needed = true;
	}
	if (SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE)) {
		palette_update_needed = true;
	}

	auto check_and_set = [&](wxCheckBox* chk, int key) {
		if (g_settings.getBoolean(key) != chk->GetValue()) {
			g_settings.setInteger(key, chk->GetValue());
			return true;
		}
		return false;
	};

	if (check_and_set(large_terrain_tools_chkbox, Config::USE_LARGE_TERRAIN_TOOLBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_collection_tools_chkbox, Config::USE_LARGE_COLLECTION_TOOLBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_doodad_sizebar_chkbox, Config::USE_LARGE_DOODAD_SIZEBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_item_sizebar_chkbox, Config::USE_LARGE_ITEM_SIZEBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_house_sizebar_chkbox, Config::USE_LARGE_HOUSE_SIZEBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_raw_sizebar_chkbox, Config::USE_LARGE_RAW_SIZEBAR)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_container_icons_chkbox, Config::USE_LARGE_CONTAINER_ICONS)) {
		palette_update_needed = true;
	}
	if (check_and_set(large_pick_item_icons_chkbox, Config::USE_LARGE_CHOOSE_ITEM_ICONS)) {
		palette_update_needed = true;
	}

	g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	float scroll_mul = 1.0;
	if (inversed_scroll_chkbox->GetValue()) {
		scroll_mul = -1.0;
	}
	g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 10.f);
	g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 10.f);

	if (palette_update_needed) {
		g_gui.RebuildPalettes();
	}
}
