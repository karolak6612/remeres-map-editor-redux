#ifndef RME_FIND_ITEM_WINDOW_H_
#define RME_FIND_ITEM_WINDOW_H_

#include "ui/find_item_window_model.h"

#include <wx/dialog.h>
#include <wx/timer.h>

#include <array>
#include <string>

class Brush;
class CreatureBrush;
class KeyForwardingTextCtrl;
class wxButton;
class wxCheckBox;
class wxRadioButton;
class wxStaticBox;

class AdvancedFinderResultListBox;

class FindItemDialog : public wxDialog {
public:
	enum SearchMode {
		ServerIDs = 0,
		ClientIDs,
		Names,
		Types,
		Properties,
	};

	enum class ActionSet : uint8_t {
		ConfirmOnly = 0,
		SearchAndSelect,
	};

	enum class ResultAction : uint8_t {
		None = 0,
		ConfirmSelection,
		SearchMap,
		SelectItem,
	};

	FindItemDialog(
		wxWindow* parent,
		const wxString& title,
		bool onlyPickupables = false,
		ActionSet action_set = ActionSet::ConfirmOnly,
		AdvancedFinderDefaultAction default_action = AdvancedFinderDefaultAction::SelectItem,
		bool include_creatures = false
	);
	~FindItemDialog() override;

	[[nodiscard]] const Brush* getResult() const {
		return result_brush_;
	}

	[[nodiscard]] uint16_t getResultID() const {
		return result_id_;
	}

	[[nodiscard]] AdvancedFinderCatalogKind getResultKind() const {
		return result_kind_;
	}

	[[nodiscard]] CreatureBrush* getResultCreatureBrush() const {
		return result_creature_brush_;
	}

	[[nodiscard]] const std::string& getResultCreatureName() const {
		return result_creature_name_;
	}

	[[nodiscard]] ResultAction getResultAction() const {
		return result_action_;
	}

	[[nodiscard]] SearchMode getSearchMode() const;
	void setSearchMode(SearchMode mode);

private:
	static constexpr size_t kFindByModeCount = 3;
	static constexpr size_t kTypeCheckboxCount = static_cast<size_t>(AdvancedFinderTypeFilter::Count);
	static constexpr size_t kPropertyCheckboxCount = static_cast<size_t>(AdvancedFinderPropertyFilter::Count);
	static constexpr size_t kInteractionCheckboxCount = static_cast<size_t>(AdvancedFinderInteractionFilter::Count);
	static constexpr size_t kVisualCheckboxCount = static_cast<size_t>(AdvancedFinderVisualFilter::Count);

	void buildLayout();
	void bindEvents();
	void loadInitialState();
	void savePersistedState();
	void applyQueryToControls() const;
	void readQueryFromControls();
	void refreshResults();
	void updateButtons();
	void updateResultTitle(size_t count) const;
	void updateCurrentSelection();
	void triggerDefaultAction();
	void handlePositiveAction(ResultAction action);
	bool shouldApplyLegacyFallback() const;

	void OnFindByChanged(wxCommandEvent& event);
	void OnFilterChanged(wxCommandEvent& event);
	void OnTextChanged(wxCommandEvent& event);
	void OnInputTimer(wxTimerEvent& event);
	void OnResultSelection(wxCommandEvent& event);
	void OnResultActivate(wxCommandEvent& event);
	void OnSearchMap(wxCommandEvent& event);
	void OnSelectItem(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnTextEnter(wxCommandEvent& event);

	wxTimer input_timer_;
	KeyForwardingTextCtrl* search_field_ = nullptr;
	std::array<wxRadioButton*, kFindByModeCount> find_by_buttons_ {};
	std::array<wxCheckBox*, kTypeCheckboxCount> type_checkboxes_ {};
	std::array<wxCheckBox*, kPropertyCheckboxCount> property_checkboxes_ {};
	std::array<wxCheckBox*, kInteractionCheckboxCount> interaction_checkboxes_ {};
	std::array<wxCheckBox*, kVisualCheckboxCount> visual_checkboxes_ {};
	AdvancedFinderResultListBox* result_list_ = nullptr;
	wxStaticBox* result_box_ = nullptr;
	wxButton* search_map_button_ = nullptr;
	wxButton* select_item_button_ = nullptr;
	wxButton* ok_button_ = nullptr;
	wxButton* cancel_button_ = nullptr;

	AdvancedFinderQuery query_;
	AdvancedFinderPersistedState persisted_state_;
	AdvancedFinderSelectionKey current_selection_;
	std::vector<AdvancedFinderCatalogRow> catalog_;
	std::vector<size_t> filtered_indices_;

	ResultAction result_action_ = ResultAction::None;
	AdvancedFinderCatalogKind result_kind_ = AdvancedFinderCatalogKind::Item;
	Brush* result_brush_ = nullptr;
	CreatureBrush* result_creature_brush_ = nullptr;
	uint16_t result_id_ = 0;
	std::string result_creature_name_;

	bool only_pickupables_ = false;
	bool include_creatures_ = false;
	bool persist_shared_state_ = false;
	ActionSet action_set_ = ActionSet::ConfirmOnly;
	AdvancedFinderDefaultAction default_action_ = AdvancedFinderDefaultAction::SelectItem;
};

#endif
