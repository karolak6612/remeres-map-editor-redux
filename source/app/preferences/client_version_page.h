#ifndef RME_PREFERENCES_CLIENT_VERSION_PAGE_H
#define RME_PREFERENCES_CLIENT_VERSION_PAGE_H

#include "preferences_page.h"
#include "app/client_version.h"
#include <wx/treectrl.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>
#include <wx/choice.h>
#include <wx/checkbox.h>

class ClientVersionPage : public PreferencesPage {
public:
	ClientVersionPage(wxWindow* parent);
	void Apply() override;

	// Public check to ensure data validity before closing dialog
	bool ValidateData();

private:
	wxSplitterWindow* client_splitter;
	wxTreeCtrl* client_tree_ctrl;
	wxPropertyGrid* client_prop_grid;
	wxButton* add_client_btn;
	wxButton* delete_client_btn;

	wxChoice* default_version_choice;
	wxCheckBox* check_sigs_chkbox;

	struct TreeItemData : public wxTreeItemData {
		ClientVersion* cv;
		TreeItemData(ClientVersion* v) :
			cv(v) { }
	};

	void PopulateClientTree();
	void SelectClient(ClientVersion* version);
	ClientVersion* GetSelectedClient();
	void UpdatePropertyValidation(wxPGProperty* prop);

	// Event Handlers
	void OnClientSelected(wxTreeEvent& event);
	void OnTreeContextMenu(wxTreeEvent& event);
	void OnDuplicateClient(wxCommandEvent& event);
	void OnPropertyChanged(wxPropertyGridEvent& event);
	void OnAddClient(wxCommandEvent& event);
	void OnDeleteClient(wxCommandEvent& event);
};

#endif
