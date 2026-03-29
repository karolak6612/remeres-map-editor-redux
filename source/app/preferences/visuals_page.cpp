#include "app/preferences/visuals_page.h"

#include "app/preferences/preferences_layout.h"
#include "item_definitions/core/item_definition_store.h"
#include "rendering/core/game_sprite.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <ranges>

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/clrpicker.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/filepicker.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/textdlg.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>

namespace {
std::string LowercaseCopy(std::string value) {
	std::ranges::transform(value, value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

std::string MatchLabel(const VisualRule& rule) {
	switch (rule.match_type) {
		case VisualMatchType::ItemId:
			return "Item ID " + std::to_string(rule.match_id);
		case VisualMatchType::ClientId:
			return "Legacy Client Visual ID " + std::to_string(rule.match_id);
		case VisualMatchType::Marker:
			return "Marker: " + rule.match_value;
		case VisualMatchType::Overlay:
			return "Overlay: " + rule.match_value;
		case VisualMatchType::Tile:
			return "Tile: " + rule.match_value;
	}
	return "Visual";
}

uint16_t ResolveClientIdFromItem(uint16_t item_id) {
	if (item_id == 0) {
		return 0;
	}
	if (const auto definition = g_item_definitions.get(item_id)) {
		return definition.clientId();
	}
	return 0;
}

wxBitmap BuildAppearanceBitmap(const VisualAppearance& appearance) {
	switch (appearance.type) {
		case VisualAppearanceType::Rgba: {
			wxBitmap bitmap(48, 48);
			wxMemoryDC dc(bitmap);
			dc.SetBackground(wxBrush(wxColour(24, 24, 24)));
			dc.Clear();
			dc.SetPen(wxPen(wxColour(70, 70, 70)));
			dc.SetBrush(wxBrush(appearance.color));
			dc.DrawRectangle(8, 8, 32, 32);
			dc.SelectObject(wxNullBitmap);
			return bitmap;
		}
		case VisualAppearanceType::SpriteId:
			if (appearance.sprite_id != 0) {
				if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(static_cast<uint32_t>(appearance.sprite_id)))) {
					return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
				}
			}
			break;
		case VisualAppearanceType::OtherItemVisual:
			if (const uint16_t client_id = ResolveClientIdFromItem(appearance.item_id); client_id != 0) {
				if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(client_id))) {
					return SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false);
				}
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (!appearance.asset_path.empty()) {
				return IMAGE_MANAGER.GetBitmap(appearance.asset_path, wxSize(48, 48), appearance.color);
			}
			break;
	}

	return wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxART_OTHER, wxSize(48, 48));
}

}

class VisualsPreviewPanel final : public wxPanel {
public:
	explicit VisualsPreviewPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(240, 128)) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		Bind(wxEVT_PAINT, &VisualsPreviewPanel::OnPaint, this);
	}

	void SetRules(std::optional<VisualRule> before, std::optional<VisualRule> after) {
		before_ = std::move(before);
		after_ = std::move(after);
		Refresh();
	}

private:
	void OnPaint(wxPaintEvent&) {
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(GetBackgroundColour()));
		dc.Clear();

		const wxRect bounds = GetClientRect();
		const int gap = FromDIP(8);
		const int width = (bounds.width - (gap * 3)) / 2;
		DrawCard(dc, wxRect(gap, gap, width, bounds.height - (gap * 2)), "Before", before_);
		DrawCard(dc, wxRect((gap * 2) + width, gap, width, bounds.height - (gap * 2)), "After", after_);
	}

	void DrawCard(wxDC& dc, const wxRect& rect, const wxString& title, const std::optional<VisualRule>& rule) {
		dc.SetPen(wxPen(wxColour(88, 88, 88)));
		dc.SetBrush(wxBrush(wxColour(20, 20, 20)));
		dc.DrawRoundedRectangle(rect, FromDIP(6));

		dc.SetTextForeground(wxColour(220, 220, 220));
		dc.DrawText(title, rect.x + FromDIP(8), rect.y + FromDIP(6));

		if (!rule.has_value()) {
			dc.SetTextForeground(wxColour(140, 140, 140));
			dc.DrawText("No visual", rect.x + FromDIP(8), rect.y + FromDIP(34));
			return;
		}

		dc.DrawBitmap(BuildAppearanceBitmap(rule->appearance), rect.x + FromDIP(8), rect.y + FromDIP(28), true);
		dc.SetTextForeground(wxColour(200, 200, 200));
		dc.DrawText(wxString::FromUTF8(rule->label), rect.x + FromDIP(64), rect.y + FromDIP(30));
		dc.SetTextForeground(wxColour(140, 140, 140));
		dc.DrawText(wxString::FromUTF8(MatchLabel(*rule)), rect.x + FromDIP(64), rect.y + FromDIP(52));
	}

	std::optional<VisualRule> before_;
	std::optional<VisualRule> after_;
};

namespace {
struct TreeItemData final : public wxTreeItemData {
	enum class Kind {
		Group,
		Rule,
		Branding,
	};

	TreeItemData(Kind item_kind, std::string item_key = {}, std::string item_group = {}) :
		kind(item_kind),
		key(std::move(item_key)),
		group(std::move(item_group)) {
	}

	Kind kind;
	std::string key;
	std::string group;
};
}

VisualsPage::VisualsPage(wxWindow* parent) : PreferencesPage(parent), working_copy_(g_visuals) {
	auto* root = new wxBoxSizer(wxVERTICAL);

	auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
	auto* add_button = new wxButton(this, wxID_ADD, "Add Item Override");
	auto* import_button = new wxButton(this, wxID_OPEN, "Import");
	auto* export_button = new wxButton(this, wxID_SAVEAS, "Export");
	reset_selected_button_ = new wxButton(this, wxID_REVERT_TO_SAVED, "Reset Selected");
	auto* reset_all_button = new wxButton(this, wxID_CLEAR, "Reset All Overrides");

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
	tree_ctrl_ = new wxTreeCtrl(left_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_LINES_AT_ROOT);
	left_sizer->Add(search_ctrl_, 0, wxEXPAND | wxALL, FromDIP(8));
	left_sizer->Add(tree_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
	left_panel->SetSizer(left_sizer);

	auto* right_panel = new wxPanel(splitter);
	auto* right_sizer = new wxBoxSizer(wxVERTICAL);
	preview_panel_ = new VisualsPreviewPanel(right_panel);
	right_sizer->Add(preview_panel_, 0, wxEXPAND | wxALL, FromDIP(10));

	editor_book_ = new wxSimplebook(right_panel, wxID_ANY);

	empty_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(empty_panel_, "Choose a Visual", "Select a catalog entry on the left or right-click something on the map and use Change Visual...");
		sizer->AddStretchSpacer();
		sizer->Add(section, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
		sizer->AddStretchSpacer();
		empty_panel_->SetSizer(sizer);
	}

	group_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(group_panel_, "Visual Group", "Group selection lets you inspect a category and reset all local overrides inside it.");
		group_label_ = PreferencesLayout::CreateBodyText(section, "", true);
		section->GetBodySizer()->Add(group_label_, 0, wxEXPAND);
		sizer->Add(section, 0, wxEXPAND | wxALL, FromDIP(10));
		group_panel_->SetSizer(sizer);
	}

	rule_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(rule_panel_, "Selected Visual", "Edit the effective visual for this slot. Changes here become local overrides only.");
		origin_label_ = PreferencesLayout::CreateBodyText(section, "", true);
		match_label_ = PreferencesLayout::CreateBodyText(section, "", false);
		validation_label_ = PreferencesLayout::CreateBodyText(section, "", false);
		label_text_ = new wxTextCtrl(section, wxID_ANY);
		appearance_choice_ = new wxChoice(section, wxID_ANY);
		appearance_choice_->Append("RGBA");
		appearance_choice_->Append("Built-in Sprite ID");
		appearance_choice_->Append("Other Item Visual");
		appearance_choice_->Append("PNG");
		appearance_choice_->Append("SVG");
		color_picker_ = new wxColourPickerCtrl(section, wxID_ANY, *wxWHITE);
		sprite_id_spin_ = new wxSpinCtrl(section, wxID_ANY);
		sprite_id_spin_->SetRange(0, 65535);
		item_id_spin_ = new wxSpinCtrl(section, wxID_ANY);
		item_id_spin_->SetRange(0, 65535);
		asset_picker_ = new wxFilePickerCtrl(section, wxID_ANY, wxEmptyString, "Choose a visual file", "PNG and SVG files (*.png;*.svg)|*.png;*.svg|All files (*.*)|*.*", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_USE_TEXTCTRL);

		PreferencesLayout::AddControlRow(section, "Origin", "Whether the current effective value comes from bundled defaults or a local override.", origin_label_, true);
		PreferencesLayout::AddControlRow(section, "Match", "What this visual override applies to.", match_label_, true);
		PreferencesLayout::AddControlRow(section, "Validation", "Broken local overrides fall back to the bundled default at render time.", validation_label_, true);
		PreferencesLayout::AddControlRow(section, "Label", "Name shown in the catalog.", label_text_, true);
		PreferencesLayout::AddControlRow(section, "Appearance", "Choose how this visual is rendered.", appearance_choice_, true);
		PreferencesLayout::AddControlRow(section, "RGBA", "Used directly for color visuals and also as the tint passed to image-based visuals.", color_picker_, true);
		PreferencesLayout::AddControlRow(section, "Sprite ID", "Built-in editor sprite id used by bundled defaults for some markers.", sprite_id_spin_, true);
		PreferencesLayout::AddControlRow(section, "Other Item ID", "Reuse the rendered look of another item id.", item_id_spin_, true);
		PreferencesLayout::AddControlRow(section, "Asset Path", "Absolute or asset-relative PNG/SVG path.", asset_picker_, true);
		sizer->Add(section, 0, wxEXPAND | wxALL, FromDIP(10));
		rule_panel_->SetSizer(sizer);
	}

	branding_panel_ = new wxPanel(editor_book_);
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		auto* section = new PreferencesSectionPanel(branding_panel_, "Branding", "Runtime identity values are loaded from the visuals TOML layer so users can override them without recompiling.");
		assets_name_text_ = new wxTextCtrl(section, wxID_ANY);
		PreferencesLayout::AddControlRow(section, "Assets Name", "Fallback DAT/SPR basename used when a client entry does not provide explicit file names.", assets_name_text_, true);
		sizer->Add(section, 0, wxEXPAND | wxALL, FromDIP(10));
		branding_panel_->SetSizer(sizer);
	}

	editor_book_->AddPage(empty_panel_, "Empty");
	editor_book_->AddPage(group_panel_, "Group");
	editor_book_->AddPage(rule_panel_, "Rule");
	editor_book_->AddPage(branding_panel_, "Branding");
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
	label_text_->Bind(wxEVT_TEXT, &VisualsPage::OnControlChanged, this);
	appearance_choice_->Bind(wxEVT_CHOICE, &VisualsPage::OnControlChanged, this);
	sprite_id_spin_->Bind(wxEVT_SPINCTRL, &VisualsPage::OnControlChanged, this);
	item_id_spin_->Bind(wxEVT_SPINCTRL, &VisualsPage::OnControlChanged, this);
	asset_picker_->Bind(wxEVT_FILEPICKER_CHANGED, &VisualsPage::OnControlChanged, this);
	assets_name_text_->Bind(wxEVT_TEXT, &VisualsPage::OnControlChanged, this);
	color_picker_->Bind(wxEVT_COLOURPICKER_CHANGED, &VisualsPage::OnColorChanged, this);
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
	if (label_text_) {
		label_text_->Unbind(wxEVT_TEXT, &VisualsPage::OnControlChanged, this);
	}
	if (appearance_choice_) {
		appearance_choice_->Unbind(wxEVT_CHOICE, &VisualsPage::OnControlChanged, this);
	}
	if (sprite_id_spin_) {
		sprite_id_spin_->Unbind(wxEVT_SPINCTRL, &VisualsPage::OnControlChanged, this);
	}
	if (item_id_spin_) {
		item_id_spin_->Unbind(wxEVT_SPINCTRL, &VisualsPage::OnControlChanged, this);
	}
	if (asset_picker_) {
		asset_picker_->Unbind(wxEVT_FILEPICKER_CHANGED, &VisualsPage::OnControlChanged, this);
	}
	if (assets_name_text_) {
		assets_name_text_->Unbind(wxEVT_TEXT, &VisualsPage::OnControlChanged, this);
	}
	if (color_picker_) {
		color_picker_->Unbind(wxEVT_COLOURPICKER_CHANGED, &VisualsPage::OnColorChanged, this);
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

		wxString label = wxString::FromUTF8(rule->label);
		if (entry.hasOverride()) {
			label += " [Override]";
		}
		tree_ctrl_->AppendItem(group_it->second, label, -1, -1, new TreeItemData(TreeItemData::Kind::Rule, rule->key, rule->group));
	}

	const wxTreeItemId branding_root = tree_ctrl_->AppendItem(root, "Branding", -1, -1, new TreeItemData(TreeItemData::Kind::Group, {}, "Branding"));
	tree_ctrl_->AppendItem(branding_root, "Assets Name", -1, -1, new TreeItemData(TreeItemData::Kind::Branding));
	tree_ctrl_->ExpandAll();

	suppress_events_ = false;

	if (!previous_key.empty()) {
		SelectRuleByKey(previous_key);
		return;
	}

	if (previous_mode == SelectionMode::Branding) {
		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = tree_ctrl_->GetFirstChild(branding_root, cookie); child.IsOk(); child = tree_ctrl_->GetNextChild(branding_root, cookie)) {
			tree_ctrl_->SelectItem(child);
			return;
		}
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
		case SelectionMode::Branding:
			editor_book_->ChangeSelection(3);
			assets_name_text_->SetValue(wxString::FromUTF8(working_copy_.GetAssetsName()));
			reset_selected_button_->SetLabel("Reset Selected");
			reset_selected_button_->Enable(working_copy_.HasAssetsNameOverride());
			break;

		case SelectionMode::Group:
			editor_book_->ChangeSelection(1);
			group_label_->SetLabel(wxString::Format("Group: %s", wxString::FromUTF8(selected_group_)));
			reset_selected_button_->SetLabel("Reset Group Overrides");
			reset_selected_button_->Enable(true);
			break;

		case SelectionMode::Rule: {
			editor_book_->ChangeSelection(2);
			reset_selected_button_->SetLabel("Reset Selected");
			const auto rule = GetSelectedRule();
			if (!rule.has_value()) {
				origin_label_->SetLabel("No rule selected");
				match_label_->SetLabel("");
				validation_label_->SetLabel("");
				reset_selected_button_->Enable(false);
				break;
			}

			origin_label_->SetLabel(working_copy_.GetUserRule(rule->key) ? "Local override" : "Bundled default");
			match_label_->SetLabel(wxString::FromUTF8(MatchLabel(*rule)));
			validation_label_->SetLabel(rule->valid ? wxString("Valid") : wxString::FromUTF8(rule->validation_error));
			label_text_->SetValue(wxString::FromUTF8(rule->label));
			appearance_choice_->SetSelection(static_cast<int>(rule->appearance.type));
			color_picker_->SetColour(rule->appearance.color);
			sprite_id_spin_->SetValue(static_cast<int>(rule->appearance.sprite_id));
			item_id_spin_->SetValue(static_cast<int>(rule->appearance.item_id));
			asset_picker_->SetPath(wxString::FromUTF8(rule->appearance.asset_path));
			UpdateControlState(*rule);
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
		preview_panel_->SetRules(std::nullopt, std::nullopt);
		return;
	}

	std::optional<VisualRule> before;
	if (const auto* default_rule = working_copy_.GetDefaultRule(selected_key_)) {
		before = *default_rule;
	}

	preview_panel_->SetRules(before, GetSelectedRule());
}

void VisualsPage::ApplyCurrentEdits() {
	if (shutting_down_ || suppress_events_) {
		return;
	}

	if (selection_mode_ == SelectionMode::Branding) {
		working_copy_.SetAssetsNameOverride(assets_name_text_->GetValue().ToStdString());
		return;
	}

	if (selection_mode_ != SelectionMode::Rule || selected_key_.empty()) {
		return;
	}

	VisualRule rule = BuildEditableRule();
	working_copy_.SetUserRule(std::move(rule));
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

VisualRule VisualsPage::BuildEditableRule() const {
	VisualRule rule;
	if (const auto* existing = working_copy_.GetEffectiveRule(selected_key_)) {
		rule = *existing;
	}

	rule.key = selected_key_;
	rule.label = label_text_->GetValue().ToStdString();
	rule.appearance.type = static_cast<VisualAppearanceType>(appearance_choice_->GetSelection());
	rule.appearance.color = color_picker_->GetColour();
	rule.appearance.sprite_id = static_cast<uint32_t>(sprite_id_spin_->GetValue());
	rule.appearance.item_id = static_cast<uint16_t>(item_id_spin_->GetValue());
	rule.appearance.asset_path = asset_picker_->GetPath().ToStdString();
	return rule;
}

void VisualsPage::UpdateControlState(const VisualRule& rule) {
	if (shutting_down_) {
		return;
	}
	const bool uses_sprite_id = rule.appearance.type == VisualAppearanceType::SpriteId;
	const bool uses_item_id = rule.appearance.type == VisualAppearanceType::OtherItemVisual;
	const bool uses_asset = rule.appearance.type == VisualAppearanceType::Png || rule.appearance.type == VisualAppearanceType::Svg;

	sprite_id_spin_->Enable(uses_sprite_id);
	item_id_spin_->Enable(uses_item_id);
	asset_picker_->Enable(uses_asset);
	color_picker_->Enable(true);
}

void VisualsPage::OnTreeSelected(wxTreeEvent& event) {
	if (shutting_down_ || suppress_events_ || !tree_ctrl_) {
		return;
	}

	ApplyCurrentEdits();

	selection_mode_ = SelectionMode::None;
	selected_key_.clear();
	selected_group_.clear();

	if (auto* data = dynamic_cast<TreeItemData*>(tree_ctrl_->GetItemData(event.GetItem()))) {
		switch (data->kind) {
			case TreeItemData::Kind::Rule:
				selection_mode_ = SelectionMode::Rule;
				selected_key_ = data->key;
				selected_group_ = data->group;
				break;
			case TreeItemData::Kind::Group:
				selection_mode_ = SelectionMode::Group;
				selected_group_ = data->group;
				break;
			case TreeItemData::Kind::Branding:
				selection_mode_ = SelectionMode::Branding;
				break;
		}
	}

	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnSearchChanged(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	PopulateTree();
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnControlChanged(wxCommandEvent&) {
	if (shutting_down_ || suppress_events_) {
		return;
	}

	ApplyCurrentEdits();
	if (selection_mode_ == SelectionMode::Rule) {
		if (const auto rule = GetSelectedRule()) {
			UpdateControlState(*rule);
		}
		PopulateTree();
		SelectRuleByKey(selected_key_);
	}
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnColorChanged(wxColourPickerEvent&) {
	if (shutting_down_ || suppress_events_) {
		return;
	}

	ApplyCurrentEdits();
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnAddItemOverride(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	wxTextEntryDialog dialog(this, "Enter an item ID to create a new override.", "Add Item Override");
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	long item_id = 0;
	if (!dialog.GetValue().ToLong(&item_id) || item_id <= 0 || item_id > 65535) {
		wxMessageBox("Enter a valid item ID.", "Visual Overrides", wxOK | wxICON_WARNING, this);
		return;
	}

	VisualRule rule = Visuals::MakeItemRule(static_cast<uint16_t>(item_id));
	working_copy_.SetUserRule(rule);
	PopulateTree();
	SelectRuleByKey(rule.key);
}

void VisualsPage::OnResetSelected(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	switch (selection_mode_) {
		case SelectionMode::Rule:
			if (!selected_key_.empty()) {
				working_copy_.RemoveUserRule(selected_key_);
				PopulateTree();
				SelectRuleByKey(selected_key_);
			}
			break;

		case SelectionMode::Group:
			RemoveGroupOverrides(selected_group_);
			PopulateTree();
			RefreshEditor();
			RefreshPreview();
			break;

		case SelectionMode::Branding:
			working_copy_.SetAssetsNameOverride({});
			RefreshEditor();
			RefreshPreview();
			break;

		case SelectionMode::None:
		default:
			break;
	}
}

void VisualsPage::OnResetAll(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	working_copy_.ClearUserOverrides();
	selected_key_.clear();
	selected_group_.clear();
	selection_mode_ = SelectionMode::None;
	PopulateTree();
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnImport(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	wxFileDialog dialog(this, "Import Visual Overrides", wxEmptyString, "visuals.toml", "TOML files (*.toml)|*.toml|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	if (!working_copy_.ImportUserOverrides(dialog.GetPath())) {
		wxMessageBox("Failed to import the selected TOML file.", "Visual Overrides", wxOK | wxICON_ERROR, this);
		return;
	}

	PopulateTree();
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnExport(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	wxFileDialog dialog(this, "Export Visual Overrides", wxEmptyString, "visuals.toml", "TOML files (*.toml)|*.toml|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	ApplyCurrentEdits();
	if (!working_copy_.ExportUserOverrides(dialog.GetPath())) {
		wxMessageBox("Failed to export the selected TOML file.", "Visual Overrides", wxOK | wxICON_ERROR, this);
	}
}

void VisualsPage::OnWindowDestroy(wxWindowDestroyEvent& event) {
	if (event.GetEventObject() == this) {
		BeginShutdown();
	}
	event.Skip();
}
