#ifndef RME_PREFERENCES_EDITOR_PAGE_H
#define RME_PREFERENCES_EDITOR_PAGE_H

#include "preferences_page.h"
#include <wx/checkbox.h>

class EditorPage : public PreferencesPage {
public:
	EditorPage(wxWindow* parent);
	void Apply() override;

private:
	wxCheckBox* group_actions_chkbox;
	wxCheckBox* duplicate_id_warn_chkbox;
	wxCheckBox* house_remove_chkbox;
	wxCheckBox* auto_assign_doors_chkbox;
	wxCheckBox* doodad_erase_same_chkbox;
	wxCheckBox* eraser_leave_unique_chkbox;
	wxCheckBox* auto_create_spawn_chkbox;
	wxCheckBox* allow_multiple_orderitems_chkbox;
	wxCheckBox* merge_move_chkbox;
	wxCheckBox* merge_paste_chkbox;
};

#endif
