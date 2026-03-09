#ifndef RME_PREFERENCES_CLIENT_VERSION_PAGE_H
#define RME_PREFERENCES_CLIENT_VERSION_PAGE_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/propgrid/propgrid.h>
#include <wx/srchctrl.h>
#include <wx/simplebook.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

#include "app/client_asset_detector.h"
#include "app/client_version.h"
#include "app/preferences/preferences_layout.h"
#include "preferences_page.h"

class ClientVersionPage : public PreferencesPage {
public:
	explicit ClientVersionPage(wxWindow* parent);
	void Apply() override;
	bool ValidateData();
	void DiscardPendingChanges();

private:
	struct TreeItemData : public wxTreeItemData {
		explicit TreeItemData(ClientVersion* value) : cv(value) { }
		ClientVersion* cv;
	};

	enum class PropertyVisualState {
		Default,
		Pending,
		Undetected,
		Saved,
	};

	void PopulateDefaultVersionChoice();
	void PopulateClientTree();
	void SelectClient(ClientVersion* version);
	ClientVersion* GetSelectedClient();
	void RefreshClientEditor();
	void RefreshSummary();
	bool ResolvePendingChanges(ClientVersion* client);
	bool IsPendingDeletion(const ClientVersion& version) const;
	bool MatchesFilter(const ClientVersion& version) const;
	int GetMajorGroup(const ClientVersion& version) const;
	void UpdatePropertyValidation(wxPGProperty* prop);
	void ApplyDetectionResult(ClientVersion& client, const ClientAssetDetectionResult& result);
	void RequestClientAssetDetection(ClientVersion& client);
	void SyncClientPropertiesToGrid(const ClientVersion& client);
	void SetPropertyState(ClientVersion* client, std::string_view property_name, PropertyVisualState state);
	void MarkSavedProperties();
	void ClearPropertyStates(ClientVersion* client);
	[[nodiscard]] PropertyVisualState GetPropertyState(const ClientVersion* client, std::string_view property_name) const;

	void OnClientSelected(wxTreeEvent& event);
	void OnSearchChanged(wxCommandEvent& event);
	void OnSearchCancelled(wxCommandEvent& event);
	void OnTreeContextMenu(wxTreeEvent& event);
	void OnDuplicateClient(wxCommandEvent& event);
	void OnPropertyChanged(wxPropertyGridEvent& event);
	void OnAddClient(wxCommandEvent& event);
	void OnDeleteClient(wxCommandEvent& event);

	wxSplitterWindow* client_splitter = nullptr;
	wxSearchCtrl* client_search_ctrl = nullptr;
	wxTreeCtrl* client_tree_ctrl = nullptr;
	wxPropertyGrid* client_prop_grid = nullptr;
	wxButton* add_client_btn = nullptr;
	wxButton* duplicate_client_btn = nullptr;
	wxButton* delete_client_btn = nullptr;

	wxChoice* default_version_choice = nullptr;
	wxCheckBox* check_sigs_chkbox = nullptr;
	wxSimplebook* detail_book = nullptr;
	wxStaticText* summary_name_label = nullptr;
	wxStaticText* summary_dirty_label = nullptr;
	ClientVersion* active_client = nullptr;
	bool ignore_tree_selection = false;
	bool suppress_property_events = false;
	wxString last_search_text;
	std::string client_filter;
	std::unordered_set<std::string> pending_deleted_client_ids;
	uint64_t next_detection_request_id = 0;
	std::unordered_map<ClientVersionID, uint64_t> pending_detection_requests_;
	std::unordered_map<const ClientVersion*, std::unordered_map<std::string, PropertyVisualState>> property_states_;
};

#endif
