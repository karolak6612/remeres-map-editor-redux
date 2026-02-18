//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include <wx/collpane.h>

#include "app/settings.h"

#include "app/client_version.h"
#include "editor/editor.h"
#include "rendering/postprocess/post_process_manager.h"

#include "ui/gui.h"

#include "ui/dialog_util.h"
#include "app/managers/version_manager.h"
#include "app/preferences.h"
#include "util/image_manager.h"
#include <charconv>
#include <wx/tokenzr.h>
#include <wx/propgrid/advprops.h>

PreferencesWindow::PreferencesWindow(wxWindow* parent, bool clientVersionSelected = false) :
	wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxSize(600, 500), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	book = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_TOP);
	// book->SetPadding(4);

	wxImageList* imageList = new wxImageList(16, 16);
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_GEAR, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_PEN_TO_SQUARE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_IMAGE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_WINDOW_MAXIMIZE, wxSize(16, 16)));
	imageList->Add(IMAGE_MANAGER.GetBitmap(ICON_GAMEPAD, wxSize(16, 16)));
	book->AssignImageList(imageList);

	book->AddPage(CreateGeneralPage(), "General", true, 0);
	book->AddPage(CreateEditorPage(), "Editor", false, 1);
	book->AddPage(CreateGraphicsPage(), "Graphics", false, 2);
	book->AddPage(CreateUIPage(), "Interface", false, 3);
	book->AddPage(CreateClientPage(), "Client Version", clientVersionSelected, 4);

	sizer->Add(book, 1, wxEXPAND | wxALL, 10);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* okBtn = newd wxButton(this, wxID_OK, "OK");
	okBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_CHECK, wxSize(16, 16)));
	subsizer->Add(okBtn, wxSizerFlags(1).Center());

	wxButton* cancelBtn = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_XMARK, wxSize(16, 16)));
	subsizer->Add(cancelBtn, wxSizerFlags(1).Border(wxALL, 5).Left().Center());

	wxButton* applyBtn = newd wxButton(this, wxID_APPLY, "Apply");
	applyBtn->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_SYNC, wxSize(16, 16)));
	subsizer->Add(applyBtn, wxSizerFlags(1).Center());

	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 10);

	SetSizerAndFit(sizer);

	int w = g_settings.getInteger(Config::PREFERENCES_WINDOW_WIDTH);
	int h = g_settings.getInteger(Config::PREFERENCES_WINDOW_HEIGHT);
	if (w > 0 && h > 0) {
		SetSize(w, h);
	} else {
		SetSize(1200, 500);
	}

	Centre(wxBOTH);
	// FindWindowById(PANE_ADVANCED_GRAPHICS, this)->GetParent()->Fit();

	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickOK, this, wxID_OK);
	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickCancel, this, wxID_CANCEL);
	Bind(wxEVT_BUTTON, &PreferencesWindow::OnClickApply, this, wxID_APPLY);
	Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, &PreferencesWindow::OnCollapsiblePane, this);

	wxIcon icon;
	icon.CopyFromBitmap(IMAGE_MANAGER.GetBitmap(ICON_GEAR, wxSize(32, 32)));
	SetIcon(icon);
}

PreferencesWindow::~PreferencesWindow() {
	int w, h;
	GetSize(&w, &h);
	g_settings.setInteger(Config::PREFERENCES_WINDOW_WIDTH, w);
	g_settings.setInteger(Config::PREFERENCES_WINDOW_HEIGHT, h);
}

wxNotebookPage* PreferencesWindow::CreateGeneralPage() {
	wxNotebookPage* general_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	show_welcome_dialog_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 5);

	always_make_backup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 5);

	update_check_on_startup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Check for updates on startup");
	update_check_on_startup_chkbox->SetValue(g_settings.getInteger(Config::USE_UPDATER) == 1);
	sizer->Add(update_check_on_startup_chkbox, 0, wxLEFT | wxTOP, 5);

	only_one_instance_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.");
	sizer->Add(only_one_instance_chkbox, 0, wxLEFT | wxTOP, 5);

	enable_tileset_editing_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	sizer->Add(enable_tileset_editing_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo queue size: "), 0);
	undo_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x10000000);
	grid_sizer->Add(undo_size_spin, 0);
	SetWindowToolTip(tmptext, undo_size_spin, "How many action you can undo, be aware that a high value will increase memory usage.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo maximum memory size (MB): "), 0);
	undo_mem_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_MEM_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 4096);
	grid_sizer->Add(undo_mem_size_spin, 0);
	SetWindowToolTip(tmptext, undo_mem_size_spin, "The approximite limit for the memory usage of the undo queue.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Worker Threads: "), 0);
	worker_threads_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::WORKER_THREADS)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64);
	grid_sizer->Add(worker_threads_spin, 0);
	SetWindowToolTip(tmptext, worker_threads_spin, "How many threads the editor will use for intensive operations. This should be equivalent to the amount of logical processors in your system.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Replace count: "), 0);
	replace_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::REPLACE_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000);
	grid_sizer->Add(replace_size_spin, 0);
	SetWindowToolTip(tmptext, replace_size_spin, "How many items you can replace on the map using the Replace Item tool.");

	sizer->Add(grid_sizer, 0, wxALL, 5);
	sizer->AddSpacer(10);

	wxString position_choices[] = { "  {x = 0, y = 0, z = 0}",
									R"(  {"x":0,"y":0,"z":0})",
									"  x, y, z",
									"  (x, y, z)",
									"  Position(x, y, z)" };
	int radio_choices = sizeof(position_choices) / sizeof(wxString);
	position_format = newd wxRadioBox(general_page, wxID_ANY, "Copy Position Format", wxDefaultPosition, wxDefaultSize, radio_choices, position_choices, 1, wxRA_SPECIFY_COLS);
	position_format->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	sizer->Add(position_format, 0, wxALL | wxEXPAND, 5);
	SetWindowToolTip(tmptext, position_format, "The position format when copying from the map.");

	general_page->SetSizerAndFit(sizer);

	return general_page;
}

wxNotebookPage* PreferencesWindow::CreateEditorPage() {
	wxNotebookPage* editor_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	group_actions_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Group same-type actions");
	group_actions_chkbox->SetValue(g_settings.getBoolean(Config::GROUP_ACTIONS));
	group_actions_chkbox->SetToolTip("This will group actions of the same type (drawing, selection..) when several take place in consecutive order.");
	sizer->Add(group_actions_chkbox, 0, wxLEFT | wxTOP, 5);

	duplicate_id_warn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Warn for duplicate IDs");
	duplicate_id_warn_chkbox->SetValue(g_settings.getBoolean(Config::WARN_FOR_DUPLICATE_ID));
	duplicate_id_warn_chkbox->SetToolTip("Warns for most kinds of duplicate IDs.");
	sizer->Add(duplicate_id_warn_chkbox, 0, wxLEFT | wxTOP, 5);

	house_remove_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "House brush removes items");
	house_remove_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_BRUSH_REMOVE_ITEMS));
	house_remove_chkbox->SetToolTip("When this option is checked, the house brush will automaticly remove items that will respawn every time the map is loaded.");
	sizer->Add(house_remove_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_assign_doors_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto-assign door ids");
	auto_assign_doors_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DOORID));
	auto_assign_doors_chkbox->SetToolTip("This will auto-assign unique door ids to all doors placed with the door brush (or doors painted over with the house brush).\nDoes NOT affect doors placed using the RAW palette.");
	sizer->Add(auto_assign_doors_chkbox, 0, wxLEFT | wxTOP, 5);

	doodad_erase_same_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Doodad brush only erases same");
	doodad_erase_same_chkbox->SetValue(g_settings.getBoolean(Config::DOODAD_BRUSH_ERASE_LIKE));
	doodad_erase_same_chkbox->SetToolTip("The doodad brush will only erase items that belongs to the current brush.");
	sizer->Add(doodad_erase_same_chkbox, 0, wxLEFT | wxTOP, 5);

	eraser_leave_unique_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Eraser leaves unique items");
	eraser_leave_unique_chkbox->SetValue(g_settings.getBoolean(Config::ERASER_LEAVE_UNIQUE));
	eraser_leave_unique_chkbox->SetToolTip("The eraser will leave containers with items in them, items with unique or action id and items.");
	sizer->Add(eraser_leave_unique_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_create_spawn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto create spawn when placing creature");
	auto_create_spawn_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_CREATE_SPAWN));
	auto_create_spawn_chkbox->SetToolTip("When this option is checked, you can place creatures without placing a spawn manually, the spawn will be place automatically.");
	sizer->Add(auto_create_spawn_chkbox, 0, wxLEFT | wxTOP, 5);

	allow_multiple_orderitems_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Prevent toporder conflict");
	allow_multiple_orderitems_chkbox->SetValue(g_settings.getBoolean(Config::RAW_LIKE_SIMONE));
	allow_multiple_orderitems_chkbox->SetToolTip("When this option is checked, you can not place several items with the same toporder on one tile using a RAW Brush.");
	sizer->Add(allow_multiple_orderitems_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	merge_move_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge move");
	merge_move_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_MOVE));
	merge_move_chkbox->SetToolTip("Moved tiles won't replace already placed tiles.");
	sizer->Add(merge_move_chkbox, 0, wxLEFT | wxTOP, 5);

	merge_paste_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge paste");
	merge_paste_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_PASTE));
	merge_paste_chkbox->SetToolTip("Pasted tiles won't replace already placed tiles.");
	sizer->Add(merge_paste_chkbox, 0, wxLEFT | wxTOP, 5);

	editor_page->SetSizerAndFit(sizer);

	return editor_page;
}

wxNotebookPage* PreferencesWindow::CreateGraphicsPage() {
	wxWindow* tmp;
	wxNotebookPage* graphics_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	hide_items_when_zoomed_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Hide items when zoomed out");
	hide_items_when_zoomed_chkbox->SetValue(g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED));
	sizer->Add(hide_items_when_zoomed_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(hide_items_when_zoomed_chkbox, "When this option is checked, \"loose\" items will be hidden when you zoom very far out.");

	icon_selection_shadow_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use icon selection shadow");
	icon_selection_shadow_chkbox->SetValue(g_settings.getBoolean(Config::USE_GUI_SELECTION_SHADOW));
	sizer->Add(icon_selection_shadow_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(icon_selection_shadow_chkbox, "When this option is checked, selected items in the palette menu will be shaded.");

	use_memcached_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use memcached sprites");
	use_memcached_chkbox->SetValue(g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES));
	sizer->Add(use_memcached_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(use_memcached_chkbox, "When this is checked, sprites will be loaded into memory at startup and unpacked at runtime. This is faster but consumes more memory.\nIf it is not checked, the editor will use less memory but there will be a performance decrease due to reading sprites from the disk.");

	anti_aliasing_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Enable Anti-Aliasing");
	anti_aliasing_chkbox->SetValue(g_settings.getBoolean(Config::ANTI_ALIASING));
	sizer->Add(anti_aliasing_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(anti_aliasing_chkbox, "Smoothens the map rendering using linear interpolation.");

	sizer->AddSpacer(10);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	// Screen Shader
	screen_shader_choice = newd wxChoice(graphics_page, wxID_ANY);
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

	tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screen Shader: ");
	subsizer->Add(tmp, 0);
	subsizer->Add(screen_shader_choice, 0);
	SetWindowToolTip(screen_shader_choice, tmp, "Apply a post-processing shader to the map view.");

	// Icon background color
	icon_background_choice = newd wxChoice(graphics_page, wxID_ANY);
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

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Icon background color: "), 0);
	subsizer->Add(icon_background_choice, 0);
	SetWindowToolTip(icon_background_choice, tmp, "This will change the background color on icons in all windows.");

	// Cursor colors
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Cursor color: "), 0);
	subsizer->Add(cursor_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE), g_settings.getInteger(Config::CURSOR_ALPHA))), 0);
	SetWindowToolTip(icon_background_choice, tmp, "The color of the main cursor on the map (while in drawing mode).");

	// Alternate cursor color
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Secondary cursor color: "), 0);
	subsizer->Add(cursor_alt_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE), g_settings.getInteger(Config::CURSOR_ALT_ALPHA))), 0);
	SetWindowToolTip(icon_background_choice, tmp, "The color of the secondary cursor on the map (for houses and flags).");

	// Screenshot dir
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot directory: "), 0);
	screenshot_directory_picker = newd wxDirPickerCtrl(graphics_page, wxID_ANY);
	subsizer->Add(screenshot_directory_picker, 1, wxEXPAND);
	wxString ss = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	screenshot_directory_picker->SetPath(ss);
	SetWindowToolTip(screenshot_directory_picker, "Screenshot taken in the editor will be saved to this directory.");

	// Screenshot format
	screenshot_format_choice = newd wxChoice(graphics_page, wxID_ANY);
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
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot format: "), 0);
	subsizer->Add(screenshot_format_choice, 0);
	SetWindowToolTip(screenshot_format_choice, tmp, "This will affect the screenshot format used by the editor.\nTo take a screenshot, press F11.");

	sizer->Add(subsizer, 1, wxEXPAND | wxALL, 5);

	// Advanced g_settings

	// FPS Settings
	sizer->AddSpacer(10);
	auto* fps_sizer = newd wxFlexGridSizer(2, 10, 10);
	fps_sizer->AddGrowableCol(1);

	fps_sizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "FPS Limit (0 = unlimited): "), 0);
	fps_limit_spin = newd wxSpinCtrl(graphics_page, wxID_ANY, i2ws(g_settings.getInteger(Config::FRAME_RATE_LIMIT)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 300);
	fps_sizer->Add(fps_limit_spin, 0);
	SetWindowToolTip(fps_limit_spin, tmp, "Limits the frame rate to save power. Set to 0 for unlimited.");

	sizer->Add(fps_sizer, 0, wxALL, 5);

	show_fps_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Show FPS Counter");
	show_fps_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_FPS_COUNTER));
	sizer->Add(show_fps_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(show_fps_chkbox, "Displays the current frame rate in the status bar.");

	graphics_page->SetSizerAndFit(sizer);

	return graphics_page;
}

wxChoice* PreferencesWindow::AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting) {
	wxStaticText* text;
	sizer->Add(text = newd wxStaticText(parent, wxID_ANY, short_description), 0);

	wxChoice* choice = newd wxChoice(parent, wxID_ANY);
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

void PreferencesWindow::SetPaletteStyleChoice(wxChoice* ctrl, int key) {
	if (ctrl->GetSelection() == 0) {
		g_settings.setString(key, "large icons");
	} else if (ctrl->GetSelection() == 1) {
		g_settings.setString(key, "small icons");
	} else if (ctrl->GetSelection() == 2) {
		g_settings.setString(key, "listbox");
	}
}

wxNotebookPage* PreferencesWindow::CreateUIPage() {
	wxNotebookPage* ui_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);
	terrain_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Terrain Palette Style:",
		"Configures the look of the terrain palette.",
		g_settings.getString(Config::PALETTE_TERRAIN_STYLE)
	);
	collection_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Collections Palette Style:",
		"Configures the look of the collections palette.",
		g_settings.getString(Config::PALETTE_COLLECTION_STYLE)
	);
	doodad_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Doodad Palette Style:",
		"Configures the look of the doodad palette.",
		g_settings.getString(Config::PALETTE_DOODAD_STYLE)
	);
	item_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Item Palette Style:",
		"Configures the look of the item palette.",
		g_settings.getString(Config::PALETTE_ITEM_STYLE)
	);
	raw_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"RAW Palette Style:",
		"Configures the look of the raw palette.",
		g_settings.getString(Config::PALETTE_RAW_STYLE)
	);

	sizer->Add(subsizer, 0, wxALL, 6);

	sizer->AddSpacer(10);

	large_terrain_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large terrain palette tool && size icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	sizer->Add(large_terrain_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_collection_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large collections palette tool && size icons");
	large_collection_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	sizer->Add(large_collection_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_doodad_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large doodad size palette icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	sizer->Add(large_doodad_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_item_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item size palette icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	sizer->Add(large_item_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_house_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large house palette size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	sizer->Add(large_house_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_raw_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large raw palette size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	sizer->Add(large_raw_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_container_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	sizer->Add(large_container_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	large_pick_item_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	sizer->Add(large_pick_item_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	switch_mousebtn_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 5);

	doubleclick_properties_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 5);

	inversed_scroll_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When this checkbox is checked, dragging the map using the center mouse button will be inversed (default RTS behaviour).");
	sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Scroll speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_scrollspeed = int(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 10);
	scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_scrollspeed, 1, std::max(true_scrollspeed, 100));
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Zoom speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_zoomspeed = int(g_settings.getFloat(Config::ZOOM_SPEED) * 10);
	zoom_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_zoomspeed, 1, std::max(true_zoomspeed, 100));
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	ui_page->SetSizerAndFit(sizer);

	return ui_page;
}

wxNotebookPage* PreferencesWindow::CreateClientPage() {
	wxNotebookPage* client_page = newd wxPanel(book, wxID_ANY);
	wxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	// Splitter for Tree and Property Grid
	client_splitter = newd wxSplitterWindow(client_page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

	// Left side: List and Buttons
	wxPanel* left_panel = newd wxPanel(client_splitter, wxID_ANY);
	wxStaticBoxSizer* left_static_sizer = newd wxStaticBoxSizer(wxVERTICAL, left_panel, "Client List");
	wxSizer* left_inner_sizer = newd wxBoxSizer(wxVERTICAL);

	client_tree_ctrl = newd wxTreeCtrl(left_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE);
	left_inner_sizer->Add(client_tree_ctrl, 1, wxEXPAND | wxBOTTOM, 5);

	wxSizer* btn_outer_sizer = newd wxBoxSizer(wxHORIZONTAL);
	add_client_btn = newd wxButton(left_panel, wxID_ANY, "+ Add");
	delete_client_btn = newd wxButton(left_panel, wxID_ANY, "- Remove");

	// Equal width for buttons
	btn_outer_sizer->AddStretchSpacer(1);
	btn_outer_sizer->Add(add_client_btn, 4, wxEXPAND | wxRIGHT, 2);
	btn_outer_sizer->Add(delete_client_btn, 4, wxEXPAND | wxLEFT, 2);
	btn_outer_sizer->AddStretchSpacer(1);

	left_inner_sizer->Add(btn_outer_sizer, 0, wxEXPAND);
	left_static_sizer->Add(left_inner_sizer, 1, wxEXPAND | wxALL, 5);
	left_panel->SetSizer(left_static_sizer);

	// Right side: Property Grid
	wxPanel* right_panel = newd wxPanel(client_splitter, wxID_ANY);
	wxStaticBoxSizer* right_static_sizer = newd wxStaticBoxSizer(wxVERTICAL, right_panel, "Client Properties");

	client_prop_grid = newd wxPropertyGrid(right_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED | wxPG_DESCRIPTION);

	right_static_sizer->Add(client_prop_grid, 1, wxEXPAND | wxALL, 5);
	right_panel->SetSizer(right_static_sizer);

	client_splitter->SplitVertically(left_panel, right_panel, 250);
	client_splitter->SetSashGravity(0.25);
	client_splitter->SetMinimumPaneSize(150);

	main_sizer->Add(client_splitter, 1, wxEXPAND | wxALL, 5);

	// General Settings (at the bottom)
	wxStaticBoxSizer* settings_sizer = newd wxStaticBoxSizer(wxVERTICAL, client_page, "Global Options");

	wxBoxSizer* def_version_sizer = newd wxBoxSizer(wxHORIZONTAL);
	def_version_sizer->Add(newd wxStaticText(client_page, wxID_ANY, "Default Client Version:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	default_version_choice = newd wxChoice(client_page, wxID_ANY);
	def_version_sizer->Add(default_version_choice, 1, wxEXPAND);

	settings_sizer->Add(def_version_sizer, 0, wxEXPAND | wxALL, 5);

	check_sigs_chkbox = newd wxCheckBox(client_page, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getInteger(Config::CHECK_SIGNATURES));
	settings_sizer->Add(check_sigs_chkbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	main_sizer->Add(settings_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	client_page->SetSizer(main_sizer);

	PopulateClientTree();

	// Binds
	client_tree_ctrl->Bind(wxEVT_TREE_SEL_CHANGING, [this](wxTreeEvent& event) {
		ClientVersion* current = GetSelectedClient();
		if (current && current->isDirty()) {
			int res = wxMessageBox("Save changes to " + current->getName() + "?", "Unsaved Changes", wxYES_NO | wxCANCEL | wxICON_QUESTION);
			if (res == wxYES) {
				current->clearDirty();
				ClientVersion::saveVersions();
			} else if (res == wxNO) {
				current->restore();
				current->clearDirty();
			} else {
				event.Veto();
				return;
			}
		}
	});
	client_tree_ctrl->Bind(wxEVT_TREE_SEL_CHANGED, &PreferencesWindow::OnClientSelected, this);
	client_tree_ctrl->Bind(wxEVT_TREE_ITEM_MENU, &PreferencesWindow::OnTreeContextMenu, this);
	client_prop_grid->Bind(wxEVT_PG_CHANGED, &PreferencesWindow::OnPropertyChanged, this);
	add_client_btn->Bind(wxEVT_BUTTON, &PreferencesWindow::OnAddClient, this);
	delete_client_btn->Bind(wxEVT_BUTTON, &PreferencesWindow::OnDeleteClient, this);

	return client_page;
}

// Event handlers!

void PreferencesWindow::OnClickOK(wxCommandEvent& event) {
	// Validation Check before closing
	for (auto* cv : ClientVersion::getAll()) {
		if (!cv->isValid()) {
			wxMessageBox("Client '" + cv->getName() + "' has invalid data (Name cannot be empty).\nPlease fix it before saving.", "Invalid Client Data", wxOK | wxICON_ERROR);
			SelectClient(cv);
			return;
		}

		// Check for duplicates
		for (auto* other : ClientVersion::getAll()) {
			if (cv != other && cv->getName() == other->getName()) {
				wxMessageBox("Client '" + cv->getName() + "' has a duplicate name.\nPlease rename one of them before saving.", "Duplicate Client Name", wxOK | wxICON_ERROR);
				SelectClient(cv);
				return;
			}
		}
	}
	Apply();
	EndModal(wxID_OK);
}

void PreferencesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void PreferencesWindow::OnClickApply(wxCommandEvent& WXUNUSED(event)) {
	// Validation Check
	for (auto* cv : ClientVersion::getAll()) {
		if (!cv->isValid()) {
			wxMessageBox("Client '" + cv->getName() + "' has invalid data (Name cannot be empty).\nPlease fix it before saving.", "Invalid Client Data", wxOK | wxICON_ERROR);
			SelectClient(cv);
			return;
		}

		// Check for duplicates
		for (auto* other : ClientVersion::getAll()) {
			if (cv != other && cv->getName() == other->getName()) {
				wxMessageBox("Client '" + cv->getName() + "' has a duplicate name.\nPlease rename one of them before saving.", "Duplicate Client Name", wxOK | wxICON_ERROR);
				SelectClient(cv);
				return;
			}
		}
	}
	Apply();
}

void PreferencesWindow::OnCollapsiblePane(wxCollapsiblePaneEvent& event) {
	auto* win = (wxWindow*)event.GetEventObject();
	win->GetParent()->Fit();
}

void PreferencesWindow::PopulateClientTree() {
	client_tree_ctrl->DeleteAllItems();
	wxTreeItemId root = client_tree_ctrl->AddRoot("Clients");

	// Store the currently selected client to re-select it after repopulating
	ClientVersion* current_selected_client = GetSelectedClient();
	wxTreeItemId item_to_select;

	// Group clients by major version
	std::map<int, std::vector<ClientVersion*>> grouped_versions;
	ClientVersionList all_versions = ClientVersion::getAll();
	for (auto* version : all_versions) {
		int protocol = version->getVersion();
		int major;
		if (protocol >= 10000) { // e.g., 12850 -> 12
			major = protocol / 1000;
		} else if (protocol >= 1000) { // e.g., 1098 -> 10
			major = protocol / 100;
		} else if (protocol >= 100) { // e.g., 860 -> 8
			major = protocol / 100;
		} else { // For very old versions if any, e.g., 740 -> 7
			major = protocol / 10;
		}
		grouped_versions[major].push_back(version);
	}

	default_version_choice->Clear();

	// Populate the tree and default version choice
	for (const auto& [major_version, versions_in_group] : grouped_versions) {
		wxTreeItemId group = client_tree_ctrl->AppendItem(root, wxString::Format("%d.x", major_version));
		for (auto* version : versions_in_group) {
			default_version_choice->Append(wxstr(version->getName()));
			if (version->getProtocolID() == g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION)) {
				default_version_choice->SetSelection(default_version_choice->GetCount() - 1);
			}

			wxTreeItemId item = client_tree_ctrl->AppendItem(group, wxstr(version->getName()), -1, -1, new TreeItemData(version));
			if (version == current_selected_client) {
				item_to_select = item;
			}
		}
		// Collapse the group by default
		client_tree_ctrl->Collapse(group);
	}

	if (item_to_select.IsOk()) {
		client_tree_ctrl->EnsureVisible(item_to_select);
		client_tree_ctrl->SelectItem(item_to_select);
	} else {
		// If nothing specific selected, maybe expand the first group so it's not empty
		wxTreeItemIdValue cookie;
		wxTreeItemId firstGroup = client_tree_ctrl->GetFirstChild(root, cookie);
		if (firstGroup.IsOk()) {
			client_tree_ctrl->Expand(firstGroup);
		}
	}
}

ClientVersion* PreferencesWindow::GetSelectedClient() {
	wxTreeItemId selection = client_tree_ctrl->GetSelection();
	if (!selection.IsOk()) {
		return nullptr;
	}

	TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(selection);
	return data ? data->cv : nullptr;
}

void PreferencesWindow::SelectClient(ClientVersion* version) {
	if (!version) {
		client_tree_ctrl->UnselectAll();
		return;
	}

	wxTreeItemId root = client_tree_ctrl->GetRootItem();
	if (!root.IsOk()) {
		return;
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId group_item = client_tree_ctrl->GetFirstChild(root, cookie);
	while (group_item.IsOk()) {
		wxTreeItemIdValue child_cookie;
		wxTreeItemId client_item = client_tree_ctrl->GetFirstChild(group_item, child_cookie);
		while (client_item.IsOk()) {
			TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(client_item);
			if (data && data->cv == version) {
				client_tree_ctrl->Expand(group_item); // Expand the group to make it visible
				client_tree_ctrl->SelectItem(client_item);
				client_tree_ctrl->EnsureVisible(client_item);
				return;
			}
			client_item = client_tree_ctrl->GetNextChild(group_item, child_cookie);
		}
		group_item = client_tree_ctrl->GetNextSibling(group_item);
	}
}

void PreferencesWindow::OnClientSelected(wxTreeEvent& WXUNUSED(event)) {
	// First, check if we need to save changes to the PREVIOUSLY selected client
	// This is a bit tricky because the selection has ALREADY changed in the tree,
	// but we can track the current client in a member variable.

	static ClientVersion* lastSelected = nullptr;
	if (lastSelected && lastSelected->isDirty()) {
		int res = wxMessageBox("The client '" + lastSelected->getName() + "' has unsaved changes.\nWould you like to save them?", "Unsaved Changes", wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (res == wxYES) {
			lastSelected->clearDirty();
			ClientVersion::saveVersions();
		} else if (res == wxNO) {
			lastSelected->restore(); // Revert back to backup
			lastSelected->clearDirty();
		} else if (res == wxCANCEL) {
			// Selection has already changed in the tree, we can't easily "cancel" the tree selection
			// without complex logic. For now we just allow the switch but maybe we should've used OnSelChanging.
		}
	}

	ClientVersion* cv = GetSelectedClient();
	lastSelected = cv;

	if (!cv) {
		client_prop_grid->Clear();
		return;
	}

	cv->backup(); // Cache current values for "Discard"

	client_prop_grid->Clear();

	// Group: General
	client_prop_grid->Append(new wxPropertyCategory("General Info", "General"));
	client_prop_grid->Append(new wxIntProperty("Version ID", "Version", cv->getVersion()))
		->SetHelpString("The internal numeric version of the client (e.g., 860, 1077, 1285).");
	client_prop_grid->Append(new wxStringProperty("Display Name", "Name", cv->getName()))
		->SetHelpString("The name displayed in lists and menus.");
	client_prop_grid->Append(new wxStringProperty("Description", "description", cv->getDescription()))
		->SetHelpString("Provide an optional description for this client.");

	// Group: Files & Paths
	client_prop_grid->Append(new wxPropertyCategory("Files & Paths", "Files"));

	client_prop_grid->Append(new wxDirProperty("Client Path", "clientPath", cv->getClientPath().GetFullPath()))
		->SetHelpString("Selection of the client folder (where Tibia.dat and Tibia.spr are located).");

	client_prop_grid->Append(new wxStringProperty("Data Directory", "dataDirectory", cv->getDataDirectory()))
		->SetHelpString("RME's internal folder for this client's data.");
	client_prop_grid->Append(new wxStringProperty("Metadata File (.dat)", "metadataFile", cv->getMetadataFile()));
	client_prop_grid->Append(new wxStringProperty("Sprites File (.spr)", "spritesFile", cv->getSpritesFile()));

	// Group: Signatures
	client_prop_grid->Append(new wxPropertyCategory("Signatures", "Signatures"));

	wxString datSig = wxString::Format("%X", cv->getDatSignature());
	wxString sprSig = wxString::Format("%X", cv->getSprSignature());

	client_prop_grid->Append(new wxStringProperty("DAT Signature", "datSignature", datSig))
		->SetHelpString("Hex signature of the Tibia.dat file.");
	client_prop_grid->Append(new wxStringProperty("SPR Signature", "sprSignature", sprSig))
		->SetHelpString("Hex signature of the Tibia.spr file.");

	// Group: OTB Settings
	client_prop_grid->Append(new wxPropertyCategory("OTB & Map Compatibility", "OTB"));
	client_prop_grid->Append(new wxIntProperty("OTB ID", "otbId", cv->getOtbId()));
	client_prop_grid->Append(new wxIntProperty("OTB Major Version", "otbMajor", cv->getOtbMajor()));

	wxString otbmVersStr;
	for (auto v : cv->getMapVersionsSupported()) {
		if (!otbmVersStr.IsEmpty()) {
			otbmVersStr += ", ";
		}
		otbmVersStr += std::to_string((int)v + 1);
	}
	client_prop_grid->Append(new wxStringProperty("Supported OTBM Versions", "otbmVersions", otbmVersStr))
		->SetHelpString("Comma separated list of OTBM versions supporting this client (1, 2, 3, 4).");

	// Group: Config / Flags
	client_prop_grid->Append(new wxPropertyCategory("Configuration & Flags", "Config"));
	client_prop_grid->Append(new wxStringProperty("Configuration Type", "configType", cv->getConfigType()));

	client_prop_grid->Append(new wxBoolProperty("Transparency", "transparency", cv->isTransparent()));
	client_prop_grid->Append(new wxBoolProperty("Extended", "extended", cv->isExtended()));
	client_prop_grid->Append(new wxBoolProperty("Frame Durations", "frameDurations", cv->hasFrameDurations()));
	client_prop_grid->Append(new wxBoolProperty("Frame Groups", "frameGroups", cv->hasFrameGroups()));

	// Perform initial validation coloring
	wxPropertyGridIterator it = client_prop_grid->GetIterator();
	for (; !it.AtEnd(); it++) {
		UpdatePropertyValidation(*it);
	}
}

void PreferencesWindow::UpdatePropertyValidation(wxPGProperty* prop) {
	if (!prop) {
		return;
	}
	wxString name = prop->GetName();
	wxAny value = prop->GetValue();
	bool invalid = false;

	if (name == "Name") {
		wxString valStr = value.As<wxString>();
		if (valStr.IsEmpty()) {
			invalid = true;
		} else {
			// Check internal duplicate
			// Note: This is an O(N) check inside a property update, but N is small (clients < 100 usually).
			// We need the current client version pointer to exclude self.
			// The property doesn't give us the client pointer directly easily, but we know GetSelectedClient() is the one being edited.
			ClientVersion* current = GetSelectedClient();
			if (current) {
				for (auto* other : ClientVersion::getAll()) {
					if (other != current && other->getName() == nstr(valStr)) {
						invalid = true;
						break;
					}
				}
			}
		}
	}
	// Removed checks for Version/OtbId as requested

	if (invalid) {
		prop->SetBackgroundColour(wxColour(255, 200, 200)); // Light Red
	} else {
		// Reset to default (white or whatever theme)
		prop->SetBackgroundColour(wxNullColour);
	}
}

void PreferencesWindow::OnTreeContextMenu(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk()) {
		return;
	}

	// Check if it is a client item (has data)
	TreeItemData* data = (TreeItemData*)client_tree_ctrl->GetItemData(item);
	if (!data || !data->cv) {
		return;
	}

	wxMenu menu;
	menu.Append(wxID_COPY, "Duplicate");
	menu.Bind(wxEVT_MENU, &PreferencesWindow::OnDuplicateClient, this, wxID_COPY);

	PopupMenu(&menu);
}

void PreferencesWindow::OnDuplicateClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* current = GetSelectedClient();
	if (!current) {
		return;
	}

	// Use clone
	std::unique_ptr<ClientVersion> new_cv = current->clone();

	// Modify fields as requested
	new_cv->setName(current->getName() + " (Copy)");
	// new_cv->setDescription(""); // Don't clear Description
	// new_cv->setClientPath(FileName()); // Don't clear path
	// new_cv->setDataDirectory(""); // Don't clear data directory
	// new_cv->setDatSignature(0); // Don't clear signatures
	// new_cv->setSprSignature(0); // Don't clear signatures
	// new_cv->visible = true; // Clone sets visible

	ClientVersion* ptr = new_cv.get();

	// Add to list
	ClientVersion::addVersion(std::move(new_cv));

	// Refresh tree
	PopulateClientTree();

	// Select the new client
	SelectClient(ptr);
}

void PreferencesWindow::OnPropertyChanged(wxPropertyGridEvent& event) {
	ClientVersion* cv = GetSelectedClient();
	if (!cv) {
		return;
	}

	wxPGProperty* prop = event.GetProperty();
	wxString propName = prop->GetName();
	wxAny value = prop->GetValue();

	if (propName == "Version") {
		cv->setVersion(value.As<int>());
	} else if (propName == "Name") {
		cv->setName(nstr(value.As<wxString>()));
		wxTreeItemId sel = client_tree_ctrl->GetSelection();
		if (sel.IsOk()) {
			client_tree_ctrl->SetItemText(sel, value.As<wxString>());
		}
	} else if (propName == "otbId") {
		cv->setOtbId(value.As<int>());
	} else if (propName == "otbMajor") {
		cv->setOtbMajor(value.As<int>());
	} else if (propName == "otbmVersions") {
		wxString s = value.As<wxString>();
		cv->getMapVersionsSupported().clear();
		wxStringTokenizer tokenizer(s, ", ");
		while (tokenizer.HasMoreTokens()) {
			long v;
			if (tokenizer.GetNextToken().ToLong(&v)) {
				if (v >= 1 && v <= 4) {
					cv->getMapVersionsSupported().push_back(static_cast<MapVersionID>(v - 1));
				}
			}
		}
	} else if (propName == "dataDirectory") {
		cv->setDataDirectory(nstr(value.As<wxString>()));
	} else if (propName == "clientPath") {
		cv->setClientPath(FileName(value.As<wxString>()));
	} else if (propName == "datSignature") {
		uint32_t sig;
		std::string s = nstr(value.As<wxString>());
		if (std::from_chars(s.data(), s.data() + s.size(), sig, 16).ec == std::errc()) {
			cv->setDatSignature(sig);
		}
	} else if (propName == "sprSignature") {
		uint32_t sig;
		std::string s = nstr(value.As<wxString>());
		if (std::from_chars(s.data(), s.data() + s.size(), sig, 16).ec == std::errc()) {
			cv->setSprSignature(sig);
		}
	} else if (propName == "description") {
		cv->setDescription(nstr(value.As<wxString>()));
	} else if (propName == "configType") {
		cv->setConfigType(nstr(prop->GetValueAsString()));
	} else if (propName == "metadataFile") {
		cv->setMetadataFile(nstr(value.As<wxString>()));
	} else if (propName == "spritesFile") {
		cv->setSpritesFile(nstr(value.As<wxString>()));
	} else if (propName == "transparency") {
		cv->setTransparent(value.As<bool>());
	} else if (propName == "extended") {
		cv->setExtended(value.As<bool>());
	} else if (propName == "frameDurations") {
		cv->setFrameDurations(value.As<bool>());
	} else if (propName == "frameGroups") {
		cv->setFrameGroups(value.As<bool>());
	}

	cv->markDirty();
	UpdatePropertyValidation(prop);
}

void PreferencesWindow::OnAddClient(wxCommandEvent& WXUNUSED(event)) {
	std::string newName = "New Client";
	int counter = 1;
	while (ClientVersion::get(newName)) {
		newName = "New Client " + std::to_string(counter++);
	}

	OtbVersion otb;
	otb.name = newName;
	otb.id = PROTOCOL_VERSION_NONE;
	otb.format_version = OTB_VERSION_1;

	auto cv = std::make_unique<ClientVersion>(otb, newName, "1287");
	cv->setName(newName);
	cv->setMetadataFile("Tibia.dat");
	cv->setSpritesFile("Tibia.spr");
	cv->markDirty();

	ClientVersion* cv_ptr = cv.get();
	ClientVersion::addVersion(std::move(cv));

	PopulateClientTree();

	// Select the newly added client
	SelectClient(cv_ptr);
}

void PreferencesWindow::OnDeleteClient(wxCommandEvent& WXUNUSED(event)) {
	ClientVersion* cv = GetSelectedClient();
	if (!cv) {
		return;
	}

	if (wxMessageBox("Are you sure you want to delete " + cv->getName() + "?", "Confirm Delete", wxYES_NO | wxICON_WARNING) == wxYES) {
		ClientVersion::removeVersion(cv->getName());
		ClientVersion::saveVersions();
		PopulateClientTree();
	}
}

// Stuff

void PreferencesWindow::Apply() {
	bool must_restart = false;
	bool palette_update_needed = false;

	// General
	g_settings.setInteger(Config::WELCOME_DIALOG, show_welcome_dialog_chkbox->GetValue());
	g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, always_make_backup_chkbox->GetValue());
	g_settings.setInteger(Config::USE_UPDATER, update_check_on_startup_chkbox->GetValue());
	g_settings.setInteger(Config::ONLY_ONE_INSTANCE, only_one_instance_chkbox->GetValue());
	g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	g_settings.setInteger(Config::WORKER_THREADS, worker_threads_spin->GetValue());
	g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_format->GetSelection());

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR) != enable_tileset_editing_chkbox->GetValue()) {
		palette_update_needed = true;
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());

	// Editor
	g_settings.setInteger(Config::GROUP_ACTIONS, group_actions_chkbox->GetValue());
	g_settings.setInteger(Config::WARN_FOR_DUPLICATE_ID, duplicate_id_warn_chkbox->GetValue());
	g_settings.setInteger(Config::HOUSE_BRUSH_REMOVE_ITEMS, house_remove_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_ASSIGN_DOORID, auto_assign_doors_chkbox->GetValue());
	g_settings.setInteger(Config::ERASER_LEAVE_UNIQUE, eraser_leave_unique_chkbox->GetValue());
	g_settings.setInteger(Config::DOODAD_BRUSH_ERASE_LIKE, doodad_erase_same_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_CREATE_SPAWN, auto_create_spawn_chkbox->GetValue());
	g_settings.setInteger(Config::RAW_LIKE_SIMONE, allow_multiple_orderitems_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_MOVE, merge_move_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_PASTE, merge_paste_chkbox->GetValue());

	// Graphics
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
	// g_settings.setInteger(Config::CURSOR_ALPHA, clr.Alpha());

	clr = cursor_alt_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());
	// g_settings.setInteger(Config::CURSOR_ALT_ALPHA, clr.Alpha());

	g_settings.setInteger(Config::HIDE_ITEMS_WHEN_ZOOMED, hide_items_when_zoomed_chkbox->GetValue());

	// FPS
	g_settings.setInteger(Config::FRAME_RATE_LIMIT, fps_limit_spin->GetValue());
	g_settings.setInteger(Config::SHOW_FPS_COUNTER, show_fps_chkbox->GetValue());

	// Interface
	SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE);
	SetPaletteStyleChoice(collection_palette_style_choice, Config::PALETTE_COLLECTION_STYLE);
	SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE);
	SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE);
	SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE);
	g_settings.setInteger(Config::USE_LARGE_TERRAIN_TOOLBAR, large_terrain_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_COLLECTION_TOOLBAR, large_collection_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_DOODAD_SIZEBAR, large_doodad_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_ITEM_SIZEBAR, large_item_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_HOUSE_SIZEBAR, large_house_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_RAW_SIZEBAR, large_raw_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CONTAINER_ICONS, large_container_icons_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CHOOSE_ITEM_ICONS, large_pick_item_icons_chkbox->GetValue());

	g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	float scroll_mul = 1.0;
	if (inversed_scroll_chkbox->GetValue()) {
		scroll_mul = -1.0;
	}
	g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 10.f);
	g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 10.f);

	// General Client settings
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());

	if (default_version_choice->GetSelection() != wxNOT_FOUND) {
		std::string defName = nstr(default_version_choice->GetStringSelection());
		ClientVersion* defCv = ClientVersion::get(defName);
		if (defCv) {
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, defCv->getProtocolID());
		}
	}

	// Save any dirty client versions
	bool anyDirty = false;
	for (auto* cv : ClientVersion::getAll()) {
		if (cv->isDirty()) {
			cv->clearDirty();
			anyDirty = true;
		}
	}
	if (anyDirty) {
		ClientVersion::saveVersions();
	}

	g_settings.save();

	if (palette_update_needed) {
		g_gui.RebuildPalettes();
	}

	if (must_restart) {
		wxMessageBox("Some changes require a restart of the application to take effect.", "Restart Required", wxOK | wxICON_INFORMATION);
	}
}
