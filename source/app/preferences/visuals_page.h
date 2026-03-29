#ifndef RME_PREFERENCES_VISUALS_PAGE_H
#define RME_PREFERENCES_VISUALS_PAGE_H

#include "app/preferences/preferences_page.h"
#include "app/visuals.h"

#include <optional>
#include <string>

class VisualsPreviewPanel;
class wxButton;
class wxChoice;
class wxColourPickerCtrl;
class wxCommandEvent;
class wxColourPickerEvent;
class wxFilePickerCtrl;
class wxSearchCtrl;
class wxSimplebook;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;
class wxTreeCtrl;
class wxTreeEvent;
class wxWindowDestroyEvent;

class VisualsPage : public PreferencesPage {
public:
	explicit VisualsPage(wxWindow* parent);
	~VisualsPage() override;

	void Apply() override;
	void FocusContext(const VisualEditContext& context);

private:
	enum class SelectionMode {
		None,
		Rule,
		Group,
		Branding,
	};

	void PopulateTree();
	void RefreshEditor();
	void RefreshPreview();
	void ApplyCurrentEdits();
	void SelectRuleByKey(const std::string& key);
	void RemoveGroupOverrides(const std::string& group);
	std::optional<VisualRule> GetSelectedRule() const;
	VisualRule BuildEditableRule() const;
	void UpdateControlState(const VisualRule& rule);

	void OnTreeSelected(wxTreeEvent& event);
	void OnSearchChanged(wxCommandEvent& event);
	void OnControlChanged(wxCommandEvent& event);
	void OnColorChanged(wxColourPickerEvent& event);
	void OnAddItemOverride(wxCommandEvent& event);
	void OnResetSelected(wxCommandEvent& event);
	void OnResetAll(wxCommandEvent& event);
	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void BeginShutdown();

	SelectionMode selection_mode_ = SelectionMode::None;
	std::string selected_key_;
	std::string selected_group_;
	bool suppress_events_ = false;
	bool shutting_down_ = false;
	Visuals working_copy_;

	wxSearchCtrl* search_ctrl_ = nullptr;
	wxTreeCtrl* tree_ctrl_ = nullptr;
	wxSimplebook* editor_book_ = nullptr;
	VisualsPreviewPanel* preview_panel_ = nullptr;

	wxWindow* empty_panel_ = nullptr;
	wxWindow* group_panel_ = nullptr;
	wxWindow* rule_panel_ = nullptr;
	wxWindow* branding_panel_ = nullptr;

	wxStaticText* group_label_ = nullptr;
	wxStaticText* origin_label_ = nullptr;
	wxStaticText* match_label_ = nullptr;
	wxStaticText* validation_label_ = nullptr;
	wxTextCtrl* label_text_ = nullptr;
	wxChoice* appearance_choice_ = nullptr;
	wxColourPickerCtrl* color_picker_ = nullptr;
	wxSpinCtrl* sprite_id_spin_ = nullptr;
	wxSpinCtrl* item_id_spin_ = nullptr;
	wxFilePickerCtrl* asset_picker_ = nullptr;
	wxTextCtrl* assets_name_text_ = nullptr;

	wxButton* reset_selected_button_ = nullptr;
};

#endif
