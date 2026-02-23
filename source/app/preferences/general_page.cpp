#include "app/preferences/general_page.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "ui/dialog_util.h"

GeneralPage::GeneralPage(wxWindow* parent) : PreferencesPage(parent) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	show_welcome_dialog_chkbox = newd wxCheckBox(this, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, wxSizerFlags().Border(wxLEFT | wxTOP, 5));

	always_make_backup_chkbox = newd wxCheckBox(this, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	always_make_backup_chkbox->SetToolTip("Automatically create a backup copy when saving the map.");
	sizer->Add(always_make_backup_chkbox, wxSizerFlags().Border(wxLEFT | wxTOP, 5));

	update_check_on_startup_chkbox = newd wxCheckBox(this, wxID_ANY, "Check for updates on startup");
	update_check_on_startup_chkbox->SetValue(g_settings.getInteger(Config::USE_UPDATER) == 1);
	update_check_on_startup_chkbox->SetToolTip("Check for new versions of the editor when starting.");
	sizer->Add(update_check_on_startup_chkbox, wxSizerFlags().Border(wxLEFT | wxTOP, 5));

	only_one_instance_chkbox = newd wxCheckBox(this, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.");
	sizer->Add(only_one_instance_chkbox, wxSizerFlags().Border(wxLEFT | wxTOP, 5));

	enable_tileset_editing_chkbox = newd wxCheckBox(this, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	sizer->Add(enable_tileset_editing_chkbox, wxSizerFlags().Border(wxLEFT | wxTOP, 5));

	sizer->AddSpacer(10);

	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	grid_sizer->Add(tmptext = newd wxStaticText(this, wxID_ANY, "Undo queue size: "), 0);
	undo_size_spin = newd wxSpinCtrl(this, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x10000000);
	grid_sizer->Add(undo_size_spin, 0);
	SetWindowToolTip(tmptext, undo_size_spin, "How many action you can undo, be aware that a high value will increase memory usage.");

	grid_sizer->Add(tmptext = newd wxStaticText(this, wxID_ANY, "Undo maximum memory size (MB): "), 0);
	undo_mem_size_spin = newd wxSpinCtrl(this, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_MEM_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 4096);
	grid_sizer->Add(undo_mem_size_spin, 0);
	SetWindowToolTip(tmptext, undo_mem_size_spin, "The approximate limit for the memory usage of the undo queue.");

	grid_sizer->Add(tmptext = newd wxStaticText(this, wxID_ANY, "Worker Threads: "), 0);
	worker_threads_spin = newd wxSpinCtrl(this, wxID_ANY, i2ws(g_settings.getInteger(Config::WORKER_THREADS)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64);
	grid_sizer->Add(worker_threads_spin, 0);
	SetWindowToolTip(tmptext, worker_threads_spin, "How many threads the editor will use for intensive operations. This should be equivalent to the amount of logical processors in your system.");

	grid_sizer->Add(tmptext = newd wxStaticText(this, wxID_ANY, "Replace count: "), 0);
	replace_size_spin = newd wxSpinCtrl(this, wxID_ANY, i2ws(g_settings.getInteger(Config::REPLACE_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000);
	grid_sizer->Add(replace_size_spin, 0);
	SetWindowToolTip(tmptext, replace_size_spin, "How many items you can replace on the map using the Replace Item tool.");

	sizer->Add(grid_sizer, wxSizerFlags().Border(wxALL, 5));
	sizer->AddSpacer(10);

	wxString position_choices[] = { "  {x = 0, y = 0, z = 0}",
									R"(  {"x":0,"y":0,"z":0})",
									"  x, y, z",
									"  (x, y, z)",
									"  Position(x, y, z)" };
	int radio_choices = sizeof(position_choices) / sizeof(wxString);
	position_format = newd wxRadioBox(this, wxID_ANY, "Copy Position Format", wxDefaultPosition, wxDefaultSize, radio_choices, position_choices, 1, wxRA_SPECIFY_COLS);
	position_format->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	sizer->Add(position_format, wxSizerFlags().Expand().Border(wxALL, 5));
	SetWindowToolTip(position_format, "The position format when copying from the map.");

	SetSizerAndFit(sizer);
}

void GeneralPage::Apply() {
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
		g_gui.RebuildPalettes();
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());
}
