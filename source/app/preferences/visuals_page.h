#ifndef RME_PREFERENCES_VISUALS_PAGE_H
#define RME_PREFERENCES_VISUALS_PAGE_H

#include "app/preferences/preferences_page.h"
#include "app/visuals.h"

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

class VisualsPreviewPanel;
class wxBitmapButton;
class wxBoxSizer;
class wxButton;
class wxCommandEvent;
class wxNotebook;
class wxPanel;
class wxScrolledWindow;
class wxSearchCtrl;
class wxStaticText;
class wxWindowDestroyEvent;

class VisualsPage : public PreferencesPage {
public:
	explicit VisualsPage(wxWindow* parent);
	~VisualsPage() override;

	void Apply() override;
	void FocusContext(const VisualEditContext& context);

private:
	enum class TabId : size_t {
		Items,
		Markers,
		Overlays,
		TileZone,
		CustomItems,
		Count,
	};

	struct DisplayEntry {
		std::string key;
		VisualRule seed_rule;
		std::optional<VisualRule> current_rule;
		std::optional<VisualRule> preview_rule;
		bool current_has_override = false;
		bool preview_has_override = false;
		TabId tab = TabId::Items;
		std::string section;
	};

	struct RowWidgets {
		wxPanel* panel = nullptr;
		wxStaticText* name_label = nullptr;
		wxStaticText* badge_label = nullptr;
		VisualsPreviewPanel* current_preview = nullptr;
		VisualsPreviewPanel* preview = nullptr;
		wxBitmapButton* sprite_button = nullptr;
		wxBitmapButton* svg_button = nullptr;
		wxBitmapButton* color_button = nullptr;
		wxBitmapButton* reset_button = nullptr;
	};

	struct TabWidgets {
		wxScrolledWindow* window = nullptr;
		wxBoxSizer* content = nullptr;
	};

	void BuildLayout();
	void RebuildTabs();
	void BuildTab(TabId tab_id, const std::vector<DisplayEntry>& entries);
	void AddHeaderRow(wxWindow* parent, wxSizer* sizer);
	void AddSectionHeader(wxWindow* parent, wxSizer* sizer, const std::string& title);
	void AddVisualRow(wxWindow* parent, wxSizer* sizer, const DisplayEntry& entry);
	void RefreshAllRows();
	void RefreshRow(const DisplayEntry& entry);
	void RefreshRowButtons(const std::string& key, const VisualRule& seed_rule);
	void FocusRow(const std::string& key);
	void ApplyDraftRules();

	std::vector<DisplayEntry> BuildDisplayEntries() const;
	std::optional<VisualRule> RuleForKey(const Visuals& visuals, const std::string& key) const;
	std::optional<uint16_t> ParseItemId(const std::string& key) const;
	VisualRule NormalizeRule(VisualRule rule) const;
	DisplayEntry MakeDisplayEntry(const std::string& key) const;
	TabId ClassifyTab(const VisualRule& seed_rule, bool preview_has_override) const;
	std::string ClassifySection(const VisualRule& seed_rule, TabId tab_id, bool preview_has_override) const;
	std::string SearchTextFor(const DisplayEntry& entry) const;

	void PickSpriteFor(const std::string& key);
	void PickImageFor(const std::string& key);
	void PickColorFor(const std::string& key);
	void ResetRow(const std::string& key);
	void OnSearchChanged(wxCommandEvent& event);
	void OnAddItemOverride(wxCommandEvent& event);
	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);
	void OnResetAll(wxCommandEvent& event);
	void OnWindowDestroy(wxWindowDestroyEvent& event);
	void BeginShutdown();

	Visuals working_copy_;
	std::map<std::string, VisualRule> draft_rules_;
	std::map<std::string, RowWidgets> row_widgets_;
	std::array<TabWidgets, static_cast<size_t>(TabId::Count)> tabs_ {};
	std::string focus_key_;
	bool shutting_down_ = false;
	bool suppress_events_ = false;

	wxSearchCtrl* search_ctrl_ = nullptr;
	wxNotebook* notebook_ = nullptr;
	wxButton* add_button_ = nullptr;
	wxButton* import_button_ = nullptr;
	wxButton* export_button_ = nullptr;
	wxButton* reset_all_button_ = nullptr;
};

#endif
