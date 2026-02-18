#include "app/preferences/graphics_page.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"
#include "rendering/postprocess/post_process_manager.h"

GraphicsPage::GraphicsPage(wxWindow* parent) : PreferencesPage(parent) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxWindow* tmp;

	hide_items_when_zoomed_chkbox = newd wxCheckBox(this, wxID_ANY, "Hide items when zoomed out");
	hide_items_when_zoomed_chkbox->SetValue(g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED));
	sizer->Add(hide_items_when_zoomed_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(hide_items_when_zoomed_chkbox, "When this option is checked, \"loose\" items will be hidden when you zoom very far out.");

	icon_selection_shadow_chkbox = newd wxCheckBox(this, wxID_ANY, "Use icon selection shadow");
	icon_selection_shadow_chkbox->SetValue(g_settings.getBoolean(Config::USE_GUI_SELECTION_SHADOW));
	sizer->Add(icon_selection_shadow_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(icon_selection_shadow_chkbox, "When this option is checked, selected items in the palette menu will be shaded.");

	use_memcached_chkbox = newd wxCheckBox(this, wxID_ANY, "Use memcached sprites");
	use_memcached_chkbox->SetValue(g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES));
	sizer->Add(use_memcached_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(use_memcached_chkbox, "When this is checked, sprites will be loaded into memory at startup and unpacked at runtime. This is faster but consumes more memory.\nIf it is not checked, the editor will use less memory but there will be a performance decrease due to reading sprites from the disk.");

	anti_aliasing_chkbox = newd wxCheckBox(this, wxID_ANY, "Enable Anti-Aliasing");
	anti_aliasing_chkbox->SetValue(g_settings.getBoolean(Config::ANTI_ALIASING));
	sizer->Add(anti_aliasing_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(anti_aliasing_chkbox, "Smoothens the map rendering using linear interpolation.");

	sizer->AddSpacer(10);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	// Screen Shader
	screen_shader_choice = newd wxChoice(this, wxID_ANY);
	auto effect_names = PostProcessManager::Instance().GetEffectNames();
	for (const auto& name : effect_names) {
		screen_shader_choice->Append(name);
	}

	std::string current_shader = g_settings.getString(Config::SCREEN_SHADER);
	int selection = screen_shader_choice->FindString(current_shader);
	if (selection != wxNOT_FOUND) {
		screen_shader_choice->SetSelection(selection);
	} else {
		screen_shader_choice->SetSelection(0);
	}

	tmp = newd wxStaticText(this, wxID_ANY, "Screen Shader: ");
	subsizer->Add(tmp, 0);
	subsizer->Add(screen_shader_choice, 0);
	SetWindowToolTip(screen_shader_choice, tmp, "Apply a post-processing shader to the map view.");

	// Icon background color
	icon_background_choice = newd wxChoice(this, wxID_ANY);
	icon_background_choice->Append("Black background");
	icon_background_choice->Append("Gray background");
	icon_background_choice->Append("White background");
	if (g_settings.getInteger(Config::ICON_BACKGROUND) == 255) {
		icon_background_choice->SetSelection(2);
	} else if (g_settings.getInteger(Config::ICON_BACKGROUND) == 88) {
		icon_background_choice->SetSelection(1);
	} else {
		icon_background_choice->SetSelection(0);
	}

	subsizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "Icon background color: "), 0);
	subsizer->Add(icon_background_choice, 0);
	SetWindowToolTip(icon_background_choice, tmp, "This will change the background color on icons in all windows.");

	// Cursor colors
	subsizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "Cursor color: "), 0);
	subsizer->Add(cursor_color_pick = newd wxColourPickerCtrl(this, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE), g_settings.getInteger(Config::CURSOR_ALPHA))), 0);
	SetWindowToolTip(cursor_color_pick, tmp, "The color of the main cursor on the map (while in drawing mode).");

	// Alternate cursor color
	subsizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "Secondary cursor color: "), 0);
	subsizer->Add(cursor_alt_color_pick = newd wxColourPickerCtrl(this, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE), g_settings.getInteger(Config::CURSOR_ALT_ALPHA))), 0);
	SetWindowToolTip(cursor_alt_color_pick, tmp, "The color of the secondary cursor on the map (for houses and flags).");

	// Screenshot dir
	subsizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "Screenshot directory: "), 0);
	screenshot_directory_picker = newd wxDirPickerCtrl(this, wxID_ANY);
	subsizer->Add(screenshot_directory_picker, 1, wxEXPAND);
	wxString ss = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	screenshot_directory_picker->SetPath(ss);
	SetWindowToolTip(screenshot_directory_picker, "Screenshot taken in the editor will be saved to this directory.");

	// Screenshot format
	screenshot_format_choice = newd wxChoice(this, wxID_ANY);
	screenshot_format_choice->Append("PNG");
	screenshot_format_choice->Append("JPG");
	screenshot_format_choice->Append("TGA");
	screenshot_format_choice->Append("BMP");
	if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "png") {
		screenshot_format_choice->SetSelection(0);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "jpg") {
		screenshot_format_choice->SetSelection(1);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "tga") {
		screenshot_format_choice->SetSelection(2);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "bmp") {
		screenshot_format_choice->SetSelection(3);
	} else {
		screenshot_format_choice->SetSelection(0);
	}
	subsizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "Screenshot format: "), 0);
	subsizer->Add(screenshot_format_choice, 0);
	SetWindowToolTip(screenshot_format_choice, tmp, "This will affect the screenshot format used by the editor.\nTo take a screenshot, press F11.");

	sizer->Add(subsizer, 1, wxEXPAND | wxALL, 5);

	// Advanced g_settings

	// FPS Settings
	sizer->AddSpacer(10);
	auto* fps_sizer = newd wxFlexGridSizer(2, 10, 10);
	fps_sizer->AddGrowableCol(1);

	fps_sizer->Add(tmp = newd wxStaticText(this, wxID_ANY, "FPS Limit (0 = unlimited): "), 0);
	fps_limit_spin = newd wxSpinCtrl(this, wxID_ANY, i2ws(g_settings.getInteger(Config::FRAME_RATE_LIMIT)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 300);
	fps_sizer->Add(fps_limit_spin, 0);
	SetWindowToolTip(fps_limit_spin, tmp, "Limits the frame rate to save power. Set to 0 for unlimited.");

	sizer->Add(fps_sizer, 0, wxALL, 5);

	show_fps_chkbox = newd wxCheckBox(this, wxID_ANY, "Show FPS Counter");
	show_fps_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_FPS_COUNTER));
	sizer->Add(show_fps_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(show_fps_chkbox, "Displays the current frame rate in the status bar.");

	SetSizerAndFit(sizer);
}

void GraphicsPage::Apply() {
	bool must_restart = false;
	g_settings.setInteger(Config::USE_GUI_SELECTION_SHADOW, icon_selection_shadow_chkbox->GetValue());
	if (g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES) != use_memcached_chkbox->GetValue()) {
		must_restart = true;
	}
	g_settings.setInteger(Config::USE_MEMCACHED_SPRITES_TO_SAVE, use_memcached_chkbox->GetValue());

	g_settings.setInteger(Config::ANTI_ALIASING, anti_aliasing_chkbox->GetValue());
	g_settings.setString(Config::SCREEN_SHADER, nstr(screen_shader_choice->GetStringSelection()));

	if (icon_background_choice->GetSelection() == 0) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 0) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 0);
	} else if (icon_background_choice->GetSelection() == 1) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 88) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 88);
	} else if (icon_background_choice->GetSelection() == 2) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 255) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 255);
	}

	// Screenshots
	g_settings.setString(Config::SCREENSHOT_DIRECTORY, nstr(screenshot_directory_picker->GetPath()));

	std::string new_format = nstr(screenshot_format_choice->GetStringSelection());
	if (new_format == "PNG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "png");
	} else if (new_format == "TGA") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "tga");
	} else if (new_format == "JPG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "jpg");
	} else if (new_format == "BMP") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "bmp");
	}

	wxColor clr = cursor_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_BLUE, clr.Blue());

	clr = cursor_alt_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());

	g_settings.setInteger(Config::HIDE_ITEMS_WHEN_ZOOMED, hide_items_when_zoomed_chkbox->GetValue());

	// FPS
	g_settings.setInteger(Config::FRAME_RATE_LIMIT, fps_limit_spin->GetValue());
	g_settings.setInteger(Config::SHOW_FPS_COUNTER, show_fps_chkbox->GetValue());

	if (must_restart) {
		wxMessageBox("Some changes require a restart of the application to take effect.", "Restart Required", wxOK | wxICON_INFORMATION);
	}
}
