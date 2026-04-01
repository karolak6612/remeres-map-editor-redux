#ifndef RME_PREFERENCES_VISUALS_PAGE_H
#define RME_PREFERENCES_VISUALS_PAGE_H

#include "app/preferences/preferences_page.h"
#include "app/visuals.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

class VisualsPreviewPanel;
class wxButton;
class wxCommandEvent;
class wxFocusEvent;
class wxPanel;
class wxSearchCtrl;
class wxSlider;
class wxSplitterWindow;
class wxStaticBox;
class wxStaticText;
class wxTextCtrl;
class wxTreeListCtrl;
class wxTreeListEvent;
class wxWindowDestroyEvent;

class VisualsPage : public PreferencesPage {
public:
	explicit VisualsPage(wxWindow* parent);
	~VisualsPage() override;

	void Apply() override;
	void FocusContext(const VisualEditContext& context);

	enum class Category : size_t {
		Items,
		Markers,
		Overlays,
		TileZone,
		CustomItems,
		Count,
	};

private:
	struct DisplayEntry {
		std::string key;
		VisualRule seed_rule;
		std::optional<VisualRule> current_rule;
		std::optional<VisualRule> preview_rule;
		bool current_has_override = false;
		bool preview_has_override = false;
		Category category = Category::Items;
		std::string section;
	};

	struct DetailWidgets {
		wxStaticBox* configuration_box = nullptr;
		wxStaticText* selected_label = nullptr;
		wxTextCtrl* name_ctrl = nullptr;
		wxStaticText* item_id_label = nullptr;
		wxStaticText* visual_id_label = nullptr;
		wxStaticText* source_label = nullptr;
		wxStaticText* tint_label = nullptr;
		wxStaticText* image_path_label = nullptr;
		VisualsPreviewPanel* current_preview = nullptr;
		VisualsPreviewPanel* preview = nullptr;
		wxButton* sprite_button = nullptr;
		wxButton* image_button = nullptr;
		wxButton* tint_button = nullptr;
		wxButton* remove_button = nullptr;
		wxSlider* alpha_slider = nullptr;
		wxStaticText* alpha_value_label = nullptr;
		wxStaticText* apply_hint_label = nullptr;
	};

	void BuildLayout();
	void BuildLeftPane(wxWindow* parent);
	void BuildRightPane(wxWindow* parent);
	void RebuildTree();
	void RefreshDetail();
	void RefreshDetailButtons(const std::optional<VisualRule>& preview_rule, const VisualRule& seed_rule);
	void RefreshDetailMetadata(const DisplayEntry& entry);
	void RefreshAlphaControls(const std::optional<VisualRule>& preview_rule);
	void FocusRow(const std::string& key);
	void SelectKey(const std::string& key);
	void ApplyDraftRules();
	void ActivateSelectedEntry();

	static wxString BuildTypeLabel(const std::optional<VisualRule>& rule);
	static wxString BuildItemIdLabel(const DisplayEntry& entry);
	static wxString BuildVisualIdLabel(const std::optional<VisualRule>& rule);
	static wxString BuildVisualName(const DisplayEntry& entry);
	static wxString BuildSourceLabel(const std::optional<VisualRule>& rule);
	static wxString BuildTintLabel(const std::optional<VisualRule>& rule);
	static wxString BuildImagePathLabel(const std::optional<VisualRule>& rule);

	std::vector<DisplayEntry> BuildDisplayEntries() const;
	std::optional<VisualRule> RuleForKey(const Visuals& visuals, const std::string& key) const;
	std::optional<uint16_t> ParseItemId(const std::string& key) const;
	VisualRule NormalizeRule(VisualRule rule) const;
	DisplayEntry MakeDisplayEntry(const std::string& key) const;
	Category ClassifyCategory(const VisualRule& seed_rule, bool preview_has_override) const;
	std::string ClassifySection(const VisualRule& seed_rule, Category category, bool preview_has_override) const;
	std::string SearchTextFor(const DisplayEntry& entry) const;

	void PickSpriteFor(const std::string& key);
	void PickImageFor(const std::string& key);
	void PickColorFor(const std::string& key);
	void ResetRow(const std::string& key);
	void PickSpriteForSelected();
	void PickImageForSelected();
	void PickColorForSelected();
	void ResetSelected();
	void CommitNameChange();
	void ApplyAlphaForSelected(int alpha);

	void OnSearchChanged(wxCommandEvent& event);
	void OnTreeSelectionChanged(wxTreeListEvent& event);
	void OnNameCommit(wxCommandEvent& event);
	void OnNameFocusLost(wxFocusEvent& event);
	void OnAlphaChanged(wxCommandEvent& event);
	void OnAddItemOverride(wxCommandEvent& event);
	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnResetAll(wxCommandEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void BeginShutdown();

	Visuals working_copy_;
	std::map<std::string, VisualRule> draft_rules_;
	std::string selected_key_;
	std::string focus_key_;
	bool shutting_down_ = false;
	bool suppress_events_ = false;

	wxSearchCtrl* search_ctrl_ = nullptr;
	wxButton* add_button_ = nullptr;
	wxButton* import_button_ = nullptr;
	wxButton* export_button_ = nullptr;
	wxButton* reset_all_button_ = nullptr;
	wxStaticText* reset_all_hint_ = nullptr;
	wxSplitterWindow* splitter_ = nullptr;
	wxTreeListCtrl* tree_ = nullptr;
	wxPanel* detail_panel_ = nullptr;
	DetailWidgets detail_ {};
};

#endif
