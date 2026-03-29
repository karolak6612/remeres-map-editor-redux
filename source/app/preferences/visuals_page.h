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
class wxPanel;
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
	};

	void PopulateTree();
	void RefreshEditor();
	void RefreshPreview();
	void ApplyCurrentEdits();
	void SelectRuleByKey(const std::string& key);
	void RemoveGroupOverrides(const std::string& group);
	std::optional<VisualRule> GetSelectedRule() const;
	std::optional<VisualRule> GetCurrentRule() const;
	VisualRule BuildEditableRule() const;
	void UpdateControlState(const VisualRule& rule);
	void RefreshSelectedRuleState();

	void OnTreeSelected(wxTreeEvent& event);
	void OnSearchChanged(wxCommandEvent& event);
	void OnAddItemOverride(wxCommandEvent& event);
	void OnPickSprite(wxCommandEvent& event);
	void OnPickImage(wxCommandEvent& event);
	void OnPickColor(wxCommandEvent& event);
	void OnResetSelected(wxCommandEvent& event);
	void OnResetAll(wxCommandEvent& event);
	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void BeginShutdown();
	void RefreshCatalogSelectionLabel(const std::string& key);

	SelectionMode selection_mode_ = SelectionMode::None;
	std::string selected_key_;
	std::string selected_group_;
	bool suppress_events_ = false;
	bool shutting_down_ = false;
	Visuals working_copy_;

	wxSearchCtrl* search_ctrl_ = nullptr;
	wxStaticText* catalog_hint_ = nullptr;
	wxTreeCtrl* tree_ctrl_ = nullptr;
	wxSimplebook* editor_book_ = nullptr;
	VisualsPreviewPanel* current_preview_panel_ = nullptr;
	VisualsPreviewPanel* preview_panel_ = nullptr;

	wxWindow* empty_panel_ = nullptr;
	wxWindow* group_panel_ = nullptr;
	wxWindow* rule_panel_ = nullptr;

	wxStaticText* group_label_ = nullptr;
	wxStaticText* selected_rule_label_ = nullptr;
	wxStaticText* current_value_label_ = nullptr;
	wxStaticText* current_preview_label_ = nullptr;
	wxStaticText* draft_preview_label_ = nullptr;
	wxButton* sprite_button_ = nullptr;
	wxButton* svg_button_ = nullptr;
	wxButton* color_button_ = nullptr;

	wxButton* reset_selected_button_ = nullptr;
	std::optional<VisualRule> draft_rule_;
};

#endif
