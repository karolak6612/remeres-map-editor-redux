#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_color_dialog.h"
#include "app/preferences/visuals_page_helpers.h"
#include "app/preferences/visuals_page_preview.h"
#include "app/preferences/preferences_layout.h"
#include "ui/gui.h"
#include "ui/find_item_window.h"

#include <functional>
#include <map>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>
#include <wx/simplebook.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

using namespace VisualsPageHelpers;

VisualsPage::VisualsPage(wxWindow* parent) : PreferencesPage(parent), working_copy_(g_visuals) {
	auto* root = new wxBoxSizer(wxVERTICAL);

	auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
	auto* add_button = new wxButton(this, wxID_ADD, "New Item Visual...");
	auto* import_button = new wxButton(this, wxID_OPEN, "Import Overrides...");
	auto* export_button = new wxButton(this, wxID_SAVEAS, "Export Overrides...");
	reset_selected_button_ = new wxButton(this, wxID_REVERT_TO_SAVED, "Reset Selected");
	auto* reset_all_button = new wxButton(this, wxID_CLEAR, "Reset All");

	toolbar->Add(add_button, 0, wxRIGHT, FromDIP(8));
	toolbar->Add(import_button, 0, wxRIGHT, FromDIP(8));
	toolbar->Add(export_button, 0, wxRIGHT, FromDIP(8));
	toolbar->Add(reset_selected_button_, 0, wxRIGHT, FromDIP(8));
	toolbar->AddStretchSpacer();
	toolbar->Add(reset_all_button, 0);
	root->Add(toolbar, 0, wxEXPAND | wxALL, FromDIP(10));

	auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

	auto* left_panel = new wxPanel(splitter);
	auto* left_sizer = new wxBoxSizer(wxVERTICAL);
	search_ctrl_ = new wxSearchCtrl(left_panel, wxID_ANY);
	search_ctrl_->ShowSearchButton(true);
	search_ctrl_->ShowCancelButton(true);
	search_ctrl_->SetDescriptiveText("Search visuals");
	catalog_hint_ = new wxStaticText(left_panel, wxID_ANY, "Pick a catalog entry, or use right-click on the map to open a prefilled visual editor.");
	tree_ctrl_ = new wxTreeCtrl(left_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_LINES_AT_ROOT);
	left_sizer->Add(search_ctrl_, 0, wxEXPAND | wxALL, FromDIP(8));
	left_sizer->Add(catalog_hint_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
	left_sizer->Add(tree_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
	left_panel->SetSizer(left_sizer);

	auto* right_panel = new wxPanel(splitter);
	auto* right_sizer = new wxBoxSizer(wxVERTICAL);
	auto* preview_row = new wxBoxSizer(wxHORIZONTAL);
	auto* current_preview_sizer = new wxBoxSizer(wxVERTICAL);
	current_preview_label_ = PreferencesLayout::CreateBodyText(right_panel, "CURRENT", true);
	current_preview_panel_ = new VisualsPreviewPanel(right_panel);
	current_preview_sizer->Add(current_preview_label_, 0, wxBOTTOM, FromDIP(4));
	current_preview_sizer->Add(current_preview_panel_, 0);
	preview_row->Add(current_preview_sizer, 0, wxRIGHT, FromDIP(16));

	auto* draft_preview_sizer = new wxBoxSizer(wxVERTICAL);
	draft_preview_label_ = PreferencesLayout::CreateBodyText(right_panel, "PREVIEW", true);
	preview_panel_ = new VisualsPreviewPanel(right_panel);
	draft_preview_sizer->Add(draft_preview_label_, 0, wxBOTTOM, FromDIP(4));
	draft_preview_sizer->Add(preview_panel_, 0);
	preview_row->Add(draft_preview_sizer, 0);
	right_sizer->Add(preview_row, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

	editor_book_ = new wxSimplebook(right_panel, wxID_ANY);

	empty_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(empty_panel_, "Choose a Visual", "Select a catalog entry on the left, or right-click a map tile and use Change Visual...");
		sizer->AddStretchSpacer();
		sizer->Add(section, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
		sizer->AddStretchSpacer();
		empty_panel_->SetSizer(sizer);
	}

	group_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(group_panel_, "Visual Group", "Group selection lets you inspect one category and reset all local overrides inside it.");
		group_label_ = PreferencesLayout::CreateBodyText(section, "", true);
		section->GetBodySizer()->Add(group_label_, 0, wxEXPAND);
		sizer->Add(section, 0, wxEXPAND | wxALL, FromDIP(10));
		group_panel_->SetSizer(sizer);
	}

	rule_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(rule_panel_, "Selected Visual", "Pick one source. SVG / PNG can optionally be tinted with Color.");
		selected_rule_label_ = PreferencesLayout::CreateBodyText(section, "", true);
		current_value_label_ = PreferencesLayout::CreateBodyText(section, "", false);
		auto* mode_row = new wxBoxSizer(wxHORIZONTAL);
		sprite_button_ = new wxButton(section, wxID_ANY, "SPR", wxDefaultPosition, wxSize(32, 32));
		svg_button_ = new wxButton(section, wxID_ANY, "SVG", wxDefaultPosition, wxSize(32, 32));
		color_button_ = new wxButton(section, wxID_ANY, "CLR", wxDefaultPosition, wxSize(32, 32));
		mode_row->Add(sprite_button_, 0, wxRIGHT, FromDIP(8));
		mode_row->Add(svg_button_, 0, wxRIGHT, FromDIP(8));
		mode_row->Add(color_button_, 0);
		section->GetBodySizer()->Add(selected_rule_label_, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
		section->GetBodySizer()->Add(current_value_label_, 0, wxEXPAND | wxBOTTOM, FromDIP(10));
		section->GetBodySizer()->Add(mode_row, 0, wxALIGN_LEFT);
		sizer->Add(section, 0, wxEXPAND | wxALL, FromDIP(10));
		rule_panel_->SetSizer(sizer);
	}

	editor_book_->AddPage(empty_panel_, "Empty");
	editor_book_->AddPage(group_panel_, "Group");
	editor_book_->AddPage(rule_panel_, "Rule");
	right_sizer->Add(editor_book_, 1, wxEXPAND);
	right_panel->SetSizer(right_sizer);

	splitter->SplitVertically(left_panel, right_panel, FromDIP(300));
	splitter->SetSashGravity(0.30);
	splitter->SetMinimumPaneSize(FromDIP(220));
	root->Add(splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
	SetSizer(root);

	search_ctrl_->Bind(wxEVT_TEXT, &VisualsPage::OnSearchChanged, this);
	tree_ctrl_->Bind(wxEVT_TREE_SEL_CHANGED, &VisualsPage::OnTreeSelected, this);
	add_button->Bind(wxEVT_BUTTON, &VisualsPage::OnAddItemOverride, this);
	import_button->Bind(wxEVT_BUTTON, &VisualsPage::OnImport, this);
	export_button->Bind(wxEVT_BUTTON, &VisualsPage::OnExport, this);
	reset_selected_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnResetSelected, this);
	reset_all_button->Bind(wxEVT_BUTTON, &VisualsPage::OnResetAll, this);
	sprite_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnPickSprite, this);
	svg_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnPickImage, this);
	color_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnPickColor, this);
	Bind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);

	PopulateTree();
	RefreshEditor();
	RefreshPreview();
}

VisualsPage::~VisualsPage() {
	BeginShutdown();
}

void VisualsPage::BeginShutdown() {
	if (shutting_down_) {
		return;
	}

	shutting_down_ = true;
	suppress_events_ = true;

	if (tree_ctrl_) {
		tree_ctrl_->Unbind(wxEVT_TREE_SEL_CHANGED, &VisualsPage::OnTreeSelected, this);
	}
	if (search_ctrl_) {
		search_ctrl_->Unbind(wxEVT_TEXT, &VisualsPage::OnSearchChanged, this);
	}
	if (sprite_button_) {
		sprite_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnPickSprite, this);
	}
	if (svg_button_) {
		svg_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnPickImage, this);
	}
	if (color_button_) {
		color_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnPickColor, this);
	}
	if (reset_selected_button_) {
		reset_selected_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnResetSelected, this);
	}
	Unbind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);
}

void VisualsPage::Apply() {
	if (shutting_down_) {
		return;
	}
	ApplyCurrentEdits();
	g_visuals = working_copy_;
	g_visuals.SaveUserOverrides();
	g_gui.RefreshView();
}

void VisualsPage::FocusContext(const VisualEditContext& context) {
	if (shutting_down_) {
		return;
	}
	ApplyCurrentEdits();

	if (context.seed_rule.key.empty()) {
		return;
	}

	if (!working_copy_.GetEffectiveRule(context.seed_rule.key)) {
		working_copy_.SetUserRule(context.seed_rule);
	}

	PopulateTree();
	SelectRuleByKey(context.seed_rule.key);
}

void VisualsPage::PopulateTree() {
	if (shutting_down_ || !tree_ctrl_ || !search_ctrl_) {
		return;
	}
	const std::string filter = LowercaseCopy(search_ctrl_->GetValue().ToStdString());
	const std::string previous_key = selected_key_;
	const std::string previous_group = selected_group_;
	const SelectionMode previous_mode = selection_mode_;

	suppress_events_ = true;
	tree_ctrl_->DeleteAllItems();
	const wxTreeItemId root = tree_ctrl_->AddRoot("visuals");

	std::map<std::string, wxTreeItemId> groups;
	for (const auto& entry : working_copy_.BuildCatalog()) {
		const VisualRule* rule = entry.effective();
		if (!rule) {
			continue;
		}

		std::string haystack = LowercaseCopy(rule->group + " " + rule->label + " " + rule->key + " " + MatchLabel(*rule));
		if (!filter.empty() && haystack.find(filter) == std::string::npos) {
			continue;
		}

		auto [group_it, inserted] = groups.emplace(rule->group, wxTreeItemId {});
		if (inserted) {
			group_it->second = tree_ctrl_->AppendItem(root, wxString::FromUTF8(rule->group), -1, -1, new TreeItemData(TreeItemData::Kind::Group, {}, rule->group));
		}

		tree_ctrl_->AppendItem(group_it->second, CatalogLabel(entry), -1, -1, new TreeItemData(TreeItemData::Kind::Rule, rule->key, rule->group));
	}

	tree_ctrl_->ExpandAll();

	suppress_events_ = false;

	if (!previous_key.empty()) {
		SelectRuleByKey(previous_key);
		return;
	}

	if (previous_mode == SelectionMode::Group && !previous_group.empty()) {
		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = tree_ctrl_->GetFirstChild(root, cookie); child.IsOk(); child = tree_ctrl_->GetNextChild(root, cookie)) {
			if (auto* data = dynamic_cast<TreeItemData*>(tree_ctrl_->GetItemData(child)); data && data->group == previous_group) {
				tree_ctrl_->SelectItem(child);
				return;
			}
		}
	}
}

void VisualsPage::RefreshEditor() {
	if (shutting_down_) {
		return;
	}
	suppress_events_ = true;

	switch (selection_mode_) {
		case SelectionMode::Group:
			editor_book_->ChangeSelection(1);
			group_label_->SetLabel(wxString::Format("Group: %s", wxString::FromUTF8(selected_group_)));
			reset_selected_button_->SetLabel("Reset Group Overrides");
			reset_selected_button_->Enable(true);
			break;

		case SelectionMode::Rule: {
			const auto rule = GetSelectedRule();
			editor_book_->ChangeSelection(2);
			reset_selected_button_->SetLabel("Reset Selected");
			if (!rule.has_value()) {
				selected_rule_label_->SetLabel("No rule selected");
				current_value_label_->SetLabel("");
				UpdateControlState(VisualRule {});
				reset_selected_button_->Enable(false);
				break;
			}

			draft_rule_ = *rule;
			selected_rule_label_->SetLabel(wxString::FromUTF8(rule->label + "  •  " + MatchLabel(*rule)));
			current_value_label_->SetLabel(CurrentValueLabel(BuildEditableRule()));
			UpdateControlState(BuildEditableRule());
			reset_selected_button_->Enable(true);
			break;
		}

		case SelectionMode::None:
		default:
			editor_book_->ChangeSelection(0);
			reset_selected_button_->SetLabel("Reset Selected");
			reset_selected_button_->Enable(false);
			break;
	}

	suppress_events_ = false;
	Layout();
}

void VisualsPage::RefreshPreview() {
	if (shutting_down_ || !preview_panel_) {
		return;
	}
	if (selection_mode_ != SelectionMode::Rule || selected_key_.empty()) {
		if (current_preview_panel_) {
			current_preview_panel_->SetRule(std::nullopt);
		}
		preview_panel_->SetRule(std::nullopt);
		return;
	}

	if (current_preview_panel_) {
		current_preview_panel_->SetRule(GetCurrentRule());
	}
	preview_panel_->SetRule(BuildEditableRule());
}

void VisualsPage::RefreshSelectedRuleState() {
	if (shutting_down_ || selection_mode_ != SelectionMode::Rule) {
		return;
	}

	if (const auto rule = GetSelectedRule(); rule.has_value()) {
		const VisualRule editable_rule = BuildEditableRule();
		selected_rule_label_->SetLabel(wxString::FromUTF8(rule->label + "  •  " + MatchLabel(*rule)));
		current_value_label_->SetLabel(CurrentValueLabel(editable_rule));
		UpdateControlState(editable_rule);
	}
}

void VisualsPage::ApplyCurrentEdits() {
	if (shutting_down_ || suppress_events_) {
		return;
	}

	if (selection_mode_ != SelectionMode::Rule || selected_key_.empty()) {
		return;
	}

	VisualRule rule = BuildEditableRule();
	working_copy_.SetUserRule(std::move(rule));
	RefreshCatalogSelectionLabel(selected_key_);
}

void VisualsPage::SelectRuleByKey(const std::string& key) {
	if (shutting_down_ || key.empty() || !tree_ctrl_) {
		return;
	}

	const wxTreeItemId root = tree_ctrl_->GetRootItem();
	if (!root.IsOk()) {
		return;
	}

	std::function<bool(const wxTreeItemId&)> visit = [&](const wxTreeItemId& item) {
		if (auto* data = dynamic_cast<TreeItemData*>(tree_ctrl_->GetItemData(item)); data && data->kind == TreeItemData::Kind::Rule && data->key == key) {
			tree_ctrl_->SelectItem(item);
			tree_ctrl_->EnsureVisible(item);
			return true;
		}

		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = tree_ctrl_->GetFirstChild(item, cookie); child.IsOk(); child = tree_ctrl_->GetNextChild(item, cookie)) {
			if (visit(child)) {
				return true;
			}
		}
		return false;
	};

	visit(root);
}

void VisualsPage::RefreshCatalogSelectionLabel(const std::string& key) {
	if (shutting_down_ || key.empty() || !tree_ctrl_) {
		return;
	}

	const auto* rule = working_copy_.GetEffectiveRule(key);
	if (!rule) {
		return;
	}

	std::function<bool(const wxTreeItemId&)> visit = [&](const wxTreeItemId& item) {
		if (auto* data = dynamic_cast<TreeItemData*>(tree_ctrl_->GetItemData(item)); data && data->kind == TreeItemData::Kind::Rule && data->key == key) {
			VisualCatalogEntry entry;
			if (const auto* default_rule = working_copy_.GetDefaultRule(key)) {
				entry.default_rule = *default_rule;
			}
			if (const auto* user_rule = working_copy_.GetUserRule(key)) {
				entry.user_rule = *user_rule;
			}
			tree_ctrl_->SetItemText(item, CatalogLabel(entry));
			return true;
		}

		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = tree_ctrl_->GetFirstChild(item, cookie); child.IsOk(); child = tree_ctrl_->GetNextChild(item, cookie)) {
			if (visit(child)) {
				return true;
			}
		}
		return false;
	};

	visit(tree_ctrl_->GetRootItem());
}

void VisualsPage::RemoveGroupOverrides(const std::string& group) {
	if (group.empty()) {
		return;
	}

	std::vector<std::string> keys_to_remove;
	for (const auto& entry : working_copy_.BuildCatalog()) {
		const VisualRule* rule = entry.effective();
		if (!rule || !entry.hasOverride()) {
			continue;
		}
		if (rule->group == group) {
			keys_to_remove.push_back(rule->key);
		}
	}

	for (const auto& key : keys_to_remove) {
		working_copy_.RemoveUserRule(key);
	}
}

std::optional<VisualRule> VisualsPage::GetSelectedRule() const {
	if (selection_mode_ != SelectionMode::Rule || selected_key_.empty()) {
		return std::nullopt;
	}
	if (const auto* rule = working_copy_.GetEffectiveRule(selected_key_)) {
		return *rule;
	}
	return std::nullopt;
}

std::optional<VisualRule> VisualsPage::GetCurrentRule() const {
	if (selection_mode_ != SelectionMode::Rule || selected_key_.empty()) {
		return std::nullopt;
	}

	if (const auto* rule = g_visuals.GetEffectiveRule(selected_key_)) {
		return *rule;
	}

	if (const auto editable_rule = GetSelectedRule(); editable_rule.has_value() && editable_rule->match_type == VisualMatchType::ItemId) {
		return Visuals::MakeItemRule(static_cast<uint16_t>(editable_rule->match_id));
	}

	return std::nullopt;
}

VisualRule VisualsPage::BuildEditableRule() const {
	if (draft_rule_.has_value()) {
		return *draft_rule_;
	}
	if (const auto* existing = working_copy_.GetEffectiveRule(selected_key_)) {
		return *existing;
	}
	return {};
}

void VisualsPage::UpdateControlState(const VisualRule& rule) {
	if (shutting_down_) {
		return;
	}
	const bool sprite_active = rule.appearance.type == VisualAppearanceType::OtherItemVisual || rule.appearance.type == VisualAppearanceType::SpriteId;
	const bool image_active = rule.appearance.type == VisualAppearanceType::Png || rule.appearance.type == VisualAppearanceType::Svg;
	const bool color_active = rule.appearance.type == VisualAppearanceType::Rgba || (image_active && !IsNeutralColor(rule.appearance.color));
	StyleModeButton(sprite_button_, sprite_active);
	StyleModeButton(svg_button_, image_active);
	StyleModeButton(color_button_, color_active);
	Layout();
}
