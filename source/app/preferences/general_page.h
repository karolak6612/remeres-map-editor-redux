#ifndef RME_PREFERENCES_GENERAL_PAGE_H
#define RME_PREFERENCES_GENERAL_PAGE_H

#include "preferences_page.h"
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/radiobox.h>

class GeneralPage : public PreferencesPage {
public:
	GeneralPage(wxWindow* parent);
	void Apply() override;

private:
	wxCheckBox* show_welcome_dialog_chkbox;
	wxCheckBox* always_make_backup_chkbox;
	wxCheckBox* update_check_on_startup_chkbox;
	wxCheckBox* only_one_instance_chkbox;
	wxCheckBox* enable_tileset_editing_chkbox;

	wxSpinCtrl* undo_size_spin;
	wxSpinCtrl* undo_mem_size_spin;
	wxSpinCtrl* worker_threads_spin;
	wxSpinCtrl* replace_size_spin;

	wxRadioBox* position_format;
};

#endif
