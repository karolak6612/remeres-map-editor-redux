#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_page_helpers.h"
#include "app/preferences/visuals_page_preview.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <ranges>
#include <string_view>

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/treelist.h>

using namespace VisualsPageHelpers;

namespace {

constexpr std::array<const char*, 5> CategoryTitles {
	"Items",
	"Markers",
	"Overlays",
	"Tile Zone",
	"Custom Items",
};

constexpr int SplitterInitialLeftWidth = 520;
constexpr int PreviewEdge = 128;
constexpr int ActionBitmapEdge = 24;

wxStaticText* MakeSectionLabel(wxWindow* parent, const wxString& text, bool bold = false) {
	auto* label = new wxStaticText(parent, wxID_ANY, text);
	wxFont font = label->GetFont();
	if (bold) {
		font.MakeBold();
	}
	label->SetFont(font);
	return label;
}

void ConfigureActionButton(wxButton* button, std::string_view icon, const wxSize& min_size) {
	button->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(icon));
	button->SetBitmapMargins(6, 0);
	button->SetMinSize(min_size);
}

void SetActionButtonState(wxButton* button, bool active) {
	if (!button) {
		return;
	}

	wxFont font = button->GetFont();
	font.SetWeight(active ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
	button->SetFont(font);

	if (active) {
		button->SetBackgroundColour(wxColour(222, 235, 252));
		button->SetForegroundColour(wxColour(26, 74, 136));
	} else {
		button->SetBackgroundColour(wxNullColour);
		button->SetForegroundColour(wxNullColour);
	}
	button->Refresh();
}

wxStaticBoxSizer* MakeStaticBoxSizer(wxWindow* parent, const wxString& label, int orientation = wxVERTICAL) {
	auto* box = new wxStaticBox(parent, wxID_ANY, label);
	return new wxStaticBoxSizer(box, orientation);
}

std::string StripTrailingIds(std::string label) {
	const auto paren = label.rfind(" (");
	if (paren != std::string::npos && !label.empty() && label.back() == ')') {
		const auto digits_begin = paren + 2;
		if (digits_begin < label.size() - 1 && std::ranges::all_of(label.begin() + static_cast<std::ptrdiff_t>(digits_begin), label.end() - 1, [](unsigned char value) {
			return std::isdigit(value) != 0;
		})) {
			label.erase(paren);
		}
	}

	for (const std::string_view prefix : { " [Item ID ", " [Client ID " }) {
		if (const auto pos = label.rfind(prefix); pos != std::string::npos && !label.empty() && label.back() == ']') {
			label.erase(pos);
		}
	}

	while (!label.empty() && label.back() == ' ') {
		label.pop_back();
	}
	return label;
}

const char* CategoryTitle(VisualsPage::Category category) {
	return CategoryTitles[static_cast<size_t>(category)];
}

}

VisualsPage::VisualsPage(wxWindow* parent) : PreferencesPage(parent), working_copy_(g_visuals) {
	BuildLayout();
	RebuildTree();
}

VisualsPage::~VisualsPage() {
	BeginShutdown();
}

void VisualsPage::BuildLayout() {
	auto* root = new wxBoxSizer(wxVERTICAL);
	const auto dip = [this](int value) {
		return FromDIP(value);
	};

	auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
	add_button_ = new wxButton(this, wxID_ADD, "New Item Visual...");
	import_button_ = new wxButton(this, wxID_OPEN, "Import...");
	export_button_ = new wxButton(this, wxID_SAVEAS, "Export...");
	reset_all_button_ = new wxButton(this, wxID_CLEAR, "Reset All");

	toolbar->Add(add_button_, 0, wxRIGHT, dip(8));
	toolbar->Add(import_button_, 0, wxRIGHT, dip(8));
	toolbar->Add(export_button_, 0, wxRIGHT, dip(12));
	toolbar->AddStretchSpacer();
	toolbar->Add(reset_all_button_, 0);
	root->Add(toolbar, 0, wxEXPAND | wxALL, dip(10));

	splitter_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_NOBORDER);
	auto* left_panel = new wxPanel(splitter_, wxID_ANY);
	detail_panel_ = new wxPanel(splitter_, wxID_ANY);
	detail_panel_->SetMinSize(wxSize(dip(500), -1));

	BuildLeftPane(left_panel);
	BuildRightPane(detail_panel_);

	splitter_->SplitVertically(left_panel, detail_panel_, dip(SplitterInitialLeftWidth));
	splitter_->SetSashGravity(0.42);
	splitter_->SetMinimumPaneSize(dip(220));
	root->Add(splitter_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dip(10));
	SetSizer(root);

	add_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnAddItemOverride, this);
	import_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnImport, this);
	export_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnExport, this);
	reset_all_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnResetAll, this);
	tree_->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &VisualsPage::OnTreeSelectionChanged, this);
	detail_.sprite_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickSpriteForSelected(); });
	detail_.image_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickImageForSelected(); });
	detail_.tint_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickColorForSelected(); });
	detail_.restore_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ResetSelected(); });
	detail_.remove_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { RemoveSelected(); });
	detail_.name_ctrl->Bind(wxEVT_TEXT_ENTER, &VisualsPage::OnNameCommit, this);
	detail_.name_ctrl->Bind(wxEVT_KILL_FOCUS, &VisualsPage::OnNameFocusLost, this);
	detail_.alpha_slider->Bind(wxEVT_SLIDER, &VisualsPage::OnAlphaChanged, this);
	Bind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);
}

void VisualsPage::BuildLeftPane(wxWindow* parent) {
	auto* root = new wxBoxSizer(wxVERTICAL);
	tree_ = new wxTreeListCtrl(
		parent,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxTL_SINGLE | wxTL_DEFAULT_STYLE
	);
	tree_->AppendColumn("Visual", FromDIP(300), wxALIGN_LEFT, wxCOL_RESIZABLE);
	tree_->AppendColumn("Type", FromDIP(110), wxALIGN_CENTER, wxCOL_RESIZABLE);
	tree_->AppendColumn("SID", FromDIP(68), wxALIGN_CENTER, wxCOL_RESIZABLE);
	tree_->AppendColumn("CID", FromDIP(68), wxALIGN_CENTER, wxCOL_RESIZABLE);
	root->Add(tree_, 1, wxEXPAND);
	parent->SetSizer(root);
}

void VisualsPage::BuildRightPane(wxWindow* parent) {
	auto* root = new wxBoxSizer(wxVERTICAL);
	const auto dip = [this](int value) {
		return FromDIP(value);
	};
	const wxSize action_button_size(dip(136), dip(42));

	auto* configuration_box = MakeStaticBoxSizer(parent, "Configuration");
	detail_.configuration_box = configuration_box->GetStaticBox();

	auto* selected_item_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Selected item");
	auto* selected_item_row = new wxBoxSizer(wxHORIZONTAL);
	detail_.selected_label = MakeSectionLabel(selected_item_box->GetStaticBox(), "No visual selected", true);
	detail_.restore_button = new wxButton(selected_item_box->GetStaticBox(), wxID_ANY, "Restore defaults");
	detail_.remove_button = new wxButton(selected_item_box->GetStaticBox(), wxID_ANY, "Remove");
	ConfigureActionButton(detail_.restore_button, ICON_UNDO, wxSize(dip(156), dip(34)));
	ConfigureActionButton(detail_.remove_button, ICON_TRASH_CAN_SOLID, wxSize(dip(118), dip(34)));
	detail_.remove_button->SetForegroundColour(wxColour(160, 45, 45));
	selected_item_row->Add(detail_.selected_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip(10));
	selected_item_row->Add(detail_.restore_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip(8));
	selected_item_row->Add(detail_.remove_button, 0, wxALIGN_CENTER_VERTICAL);
	selected_item_box->Add(selected_item_row, 0, wxEXPAND | wxALL, dip(8));
	configuration_box->Add(selected_item_box, 0, wxEXPAND | wxALL, dip(8));

	auto* preview_row = new wxBoxSizer(wxHORIZONTAL);
	auto* current_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Current");
	auto* preview_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Preview");
	detail_.current_preview = new VisualsPreviewPanel(current_box->GetStaticBox(), PreviewEdge);
	detail_.preview = new VisualsPreviewPanel(preview_box->GetStaticBox(), PreviewEdge);
	current_box->Add(detail_.current_preview, 0, wxALL, dip(8));
	preview_box->Add(detail_.preview, 0, wxALL, dip(8));
	current_box->AddStretchSpacer();
	preview_box->AddStretchSpacer();
	preview_row->Add(current_box, 1, wxRIGHT | wxEXPAND, dip(10));
	preview_row->Add(preview_box, 1, wxEXPAND);
	configuration_box->Add(preview_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, dip(8));

	auto* source_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Source");
	auto* source_row = new wxBoxSizer(wxHORIZONTAL);
	detail_.sprite_button = new wxButton(source_box->GetStaticBox(), wxID_ANY, "Sprite");
	detail_.image_button = new wxButton(source_box->GetStaticBox(), wxID_ANY, "Image");
	ConfigureActionButton(detail_.sprite_button, ICON_OBJECT_GROUP_SOLID, action_button_size);
	ConfigureActionButton(detail_.image_button, ICON_IMAGE_SOLID, action_button_size);
	source_row->Add(detail_.sprite_button, 0, wxRIGHT, dip(10));
	source_row->Add(detail_.image_button, 0);
	source_box->Add(source_row, 0, wxALL, dip(8));
	configuration_box->Add(source_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dip(8));

	auto* tint_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Tint");
	auto* tint_row = new wxBoxSizer(wxHORIZONTAL);
	detail_.tint_button = new wxButton(tint_box->GetStaticBox(), wxID_ANY, "Tint");
	ConfigureActionButton(detail_.tint_button, ICON_PALETTE, action_button_size);
	detail_.alpha_slider = new wxSlider(tint_box->GetStaticBox(), wxID_ANY, 255, 0, 255, wxDefaultPosition, wxSize(action_button_size.x, -1));
	detail_.alpha_value_label = new wxStaticText(tint_box->GetStaticBox(), wxID_ANY, "255");
	tint_row->Add(detail_.tint_button, 0, wxRIGHT, dip(10));
	tint_row->Add(MakeSectionLabel(tint_box->GetStaticBox(), "Alpha", true), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip(8));
	tint_row->Add(detail_.alpha_slider, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, dip(8));
	tint_row->Add(detail_.alpha_value_label, 0, wxALIGN_CENTER_VERTICAL);
	tint_box->Add(tint_row, 0, wxEXPAND | wxALL, dip(8));
	configuration_box->Add(tint_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dip(8));

	auto* properties_box = MakeStaticBoxSizer(configuration_box->GetStaticBox(), "Properties");
	auto* metadata_grid = new wxFlexGridSizer(2, dip(12), dip(8));
	metadata_grid->AddGrowableCol(1, 1);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Name", true), 0, wxALIGN_CENTER_VERTICAL);
	detail_.name_ctrl = new wxTextCtrl(properties_box->GetStaticBox(), wxID_ANY, {}, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	detail_.name_ctrl->SetMinSize(wxSize(action_button_size.x, -1));
	metadata_grid->Add(detail_.name_ctrl, 1, wxEXPAND);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "SID", true), 0, wxALIGN_CENTER_VERTICAL);
	detail_.sid_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.sid_label, 0, wxALIGN_CENTER_VERTICAL);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "CID", true), 0, wxALIGN_CENTER_VERTICAL);
	detail_.cid_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.cid_label, 0, wxALIGN_CENTER_VERTICAL);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Item ID", true), 0, wxALIGN_CENTER_VERTICAL);
	detail_.item_id_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.item_id_label, 0, wxALIGN_CENTER_VERTICAL);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Visual ID", true), 0, wxALIGN_CENTER_VERTICAL);
	detail_.visual_id_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.visual_id_label, 0, wxALIGN_CENTER_VERTICAL);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Source", true), 0, wxALIGN_TOP);
	detail_.source_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.source_label, 1, wxEXPAND);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Tint", true), 0, wxALIGN_TOP);
	detail_.tint_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.tint_label, 1, wxEXPAND);
	metadata_grid->Add(MakeSectionLabel(properties_box->GetStaticBox(), "Image Path", true), 0, wxALIGN_TOP);
	detail_.image_path_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "N/A");
	metadata_grid->Add(detail_.image_path_label, 1, wxEXPAND);
	properties_box->Add(metadata_grid, 0, wxEXPAND | wxALL, dip(8));
	detail_.apply_hint_label = new wxStaticText(properties_box->GetStaticBox(), wxID_ANY, "Use Apply below to commit changes.");
	properties_box->Add(detail_.apply_hint_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, dip(8));
	configuration_box->Add(properties_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dip(8));

	root->Add(configuration_box, 0, wxEXPAND);
	root->AddStretchSpacer();

	parent->SetSizer(root);
}

void VisualsPage::Apply() {
	if (shutting_down_) {
		return;
	}

	ApplyDraftRules();
	g_visuals = working_copy_;
	g_visuals.SaveUserOverrides();
	g_visuals.PrepareRuntimeResources();
	working_copy_ = g_visuals;
	draft_rules_.clear();
	g_gui.RefreshView();
	focus_key_ = selected_key_;
	RebuildTree();
}

void VisualsPage::FocusContext(const VisualEditContext& context) {
	if (shutting_down_ || context.seed_rule.key.empty()) {
		return;
	}

	if (!working_copy_.GetEffectiveRule(context.seed_rule.key)) {
		working_copy_.SetUserRule(context.seed_rule);
	}

	focus_key_ = context.seed_rule.key;
	RebuildTree();
}

void VisualsPage::RefreshDetail() {
	if (!detail_.selected_label) {
		return;
	}

	if (selected_key_.empty()) {
		detail_.selected_label->SetLabel("No visual selected");
		detail_.name_ctrl->ChangeValue("N/A");
		detail_.sid_label->SetLabel("N/A");
		detail_.cid_label->SetLabel("N/A");
		detail_.item_id_label->SetLabel("N/A");
		detail_.visual_id_label->SetLabel("N/A");
		detail_.source_label->SetLabel("N/A");
		detail_.tint_label->SetLabel("N/A");
		detail_.image_path_label->SetLabel("N/A");
		detail_.current_preview->SetRule(std::nullopt);
		detail_.preview->SetRule(std::nullopt);
		const VisualRule empty_rule {};
		RefreshDetailButtons(std::nullopt, empty_rule);
		RefreshAlphaControls(std::nullopt);
		detail_panel_->Layout();
		return;
	}

	const DisplayEntry entry = MakeDisplayEntry(selected_key_);
	detail_.selected_label->SetLabel(BuildVisualName(entry));
	detail_.name_ctrl->ChangeValue(BuildVisualName(entry));
	RefreshDetailMetadata(entry);
	detail_.current_preview->SetRule(entry.current_rule);
	detail_.preview->SetRule(entry.preview_rule);
	RefreshDetailButtons(entry.preview_rule, entry.seed_rule);
	RefreshAlphaControls(entry.preview_rule);
	detail_panel_->Layout();
}

void VisualsPage::RefreshDetailMetadata(const DisplayEntry& entry) {
	detail_.item_id_label->SetLabel(BuildItemIdLabel(entry));
	detail_.sid_label->SetLabel(BuildSidLabel(entry));
	detail_.cid_label->SetLabel(BuildCidLabel(entry));
	detail_.visual_id_label->SetLabel(BuildVisualIdLabel(entry.preview_rule));
	detail_.source_label->SetLabel(BuildSourceLabel(entry.preview_rule));
	detail_.tint_label->SetLabel(BuildTintLabel(entry.preview_rule));
	detail_.image_path_label->SetLabel(BuildImagePathLabel(entry.preview_rule));
	const int wrap_width = FromDIP(320);
	detail_.source_label->Wrap(wrap_width);
	detail_.tint_label->Wrap(wrap_width);
	detail_.image_path_label->Wrap(wrap_width);
}

void VisualsPage::RefreshDetailButtons(const std::optional<VisualRule>& preview_rule, const VisualRule& seed_rule) {
	const bool has_selection = !selected_key_.empty();
	const bool supports_sprite = has_selection && Visuals::SupportsSpriteModes(seed_rule);
	const bool supports_image = has_selection && Visuals::SupportsImageModes(seed_rule);
	const bool restore_enabled = has_selection && (
		working_copy_.GetUserRule(selected_key_) != nullptr ||
		draft_rules_.contains(selected_key_) ||
		g_visuals.GetUserRule(selected_key_) != nullptr
	);
	const bool has_default = has_selection && working_copy_.GetDefaultRule(selected_key_) != nullptr;
	const bool remove_enabled = restore_enabled && !has_default;
	const bool sprite_active = preview_rule.has_value() && (
		preview_rule->appearance.type == VisualAppearanceType::OtherItemVisual ||
		preview_rule->appearance.type == VisualAppearanceType::SpriteId
	);
	const bool image_active = preview_rule.has_value() && (
		preview_rule->appearance.type == VisualAppearanceType::Png ||
		preview_rule->appearance.type == VisualAppearanceType::Svg
	);
	const bool tint_active = preview_rule.has_value() && (
		preview_rule->appearance.type == VisualAppearanceType::Rgba ||
		preview_rule->appearance.color != wxColour(255, 255, 255, 255)
	);

	detail_.sprite_button->Enable(supports_sprite);
	detail_.image_button->Enable(supports_image);
	detail_.tint_button->Enable(has_selection);
	detail_.restore_button->Enable(restore_enabled);
	detail_.remove_button->Enable(remove_enabled);

	if (preview_rule.has_value() && preview_rule->appearance.type == VisualAppearanceType::Svg) {
		detail_.tint_button->Enable(true);
	}

	SetActionButtonState(detail_.sprite_button, sprite_active);
	SetActionButtonState(detail_.image_button, image_active);
	SetActionButtonState(detail_.tint_button, tint_active);
}

void VisualsPage::RefreshAlphaControls(const std::optional<VisualRule>& preview_rule) {
	const bool enabled = preview_rule.has_value();
	detail_.alpha_slider->Enable(enabled);
	const int alpha = enabled && preview_rule->appearance.color.IsOk() ? preview_rule->appearance.color.Alpha() : 255;
	detail_.alpha_slider->SetValue(alpha);
	detail_.alpha_value_label->SetLabel(wxString::Format("%d", alpha));
}

wxString VisualsPage::BuildTypeLabel(const std::optional<VisualRule>& rule) {
	if (!rule.has_value()) {
		return "NONE";
	}

	switch (rule->appearance.type) {
		case VisualAppearanceType::OtherItemVisual:
		case VisualAppearanceType::SpriteId:
			return "SPR";
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
		case VisualAppearanceType::Rgba:
			return "IMG/COLOR";
	}
	return "NONE";
}

wxString VisualsPage::BuildItemIdLabel(const DisplayEntry& entry) {
	if (entry.seed_rule.match_type == VisualMatchType::ItemId && entry.seed_rule.match_id > 0) {
		return wxString::Format("%d", entry.seed_rule.match_id);
	}

	if (entry.preview_rule.has_value() && entry.preview_rule->match_type == VisualMatchType::ItemId && entry.preview_rule->match_id > 0) {
		return wxString::Format("%d", entry.preview_rule->match_id);
	}

	return "N/A";
}

wxString VisualsPage::BuildSidLabel(const DisplayEntry& entry) {
	if (entry.seed_rule.match_type == VisualMatchType::ItemId && entry.seed_rule.match_id > 0) {
		return wxString::Format("%d", entry.seed_rule.match_id);
	}

	if (entry.preview_rule.has_value() && entry.preview_rule->match_type == VisualMatchType::ItemId && entry.preview_rule->match_id > 0) {
		return wxString::Format("%d", entry.preview_rule->match_id);
	}

	return "N/A";
}

wxString VisualsPage::BuildCidLabel(const DisplayEntry& entry) {
	const auto resolve_client_id = [](const std::optional<VisualRule>& rule) -> uint16_t {
		if (!rule.has_value() || rule->match_type != VisualMatchType::ItemId || rule->match_id <= 0) {
			return 0;
		}

		if (const auto definition = g_item_definitions.get(static_cast<uint16_t>(rule->match_id))) {
			return definition.clientId();
		}
		return 0;
	};

	if (const uint16_t client_id = resolve_client_id(entry.preview_rule); client_id != 0) {
		return wxString::Format("%u", client_id);
	}
	if (const uint16_t client_id = resolve_client_id(entry.current_rule); client_id != 0) {
		return wxString::Format("%u", client_id);
	}
	return "N/A";
}

wxString VisualsPage::BuildVisualIdLabel(const std::optional<VisualRule>& rule) {
	if (!rule.has_value()) {
		return "N/A";
	}

	switch (rule->appearance.type) {
		case VisualAppearanceType::OtherItemVisual:
			return rule->appearance.item_id > 0 ? wxString::Format("%u", rule->appearance.item_id) : wxString("N/A");
		case VisualAppearanceType::SpriteId:
			return rule->appearance.sprite_id > 0 ? wxString::Format("%u", rule->appearance.sprite_id) : wxString("N/A");
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
		case VisualAppearanceType::Rgba:
			return "N/A";
	}
	return "N/A";
}

wxString VisualsPage::BuildVisualName(const DisplayEntry& entry) {
	return wxString::FromUTF8(StripTrailingIds(entry.seed_rule.label));
}

wxString VisualsPage::BuildSourceLabel(const std::optional<VisualRule>& rule) {
	if (!rule.has_value()) {
		return "N/A";
	}

	switch (rule->appearance.type) {
		case VisualAppearanceType::OtherItemVisual:
			return rule->appearance.item_id > 0 ? wxString::Format("Item %u", rule->appearance.item_id) : wxString("N/A");
		case VisualAppearanceType::SpriteId:
			return rule->appearance.sprite_id > 0 ? wxString::Format("Sprite %u", rule->appearance.sprite_id) : wxString("N/A");
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			return "Image";
		case VisualAppearanceType::Rgba:
			return "Flat color";
	}

	return "N/A";
}

wxString VisualsPage::BuildTintLabel(const std::optional<VisualRule>& rule) {
	if (!rule.has_value()) {
		return "N/A";
	}

	if (!rule->appearance.color.IsOk()) {
		return "N/A";
	}

	const wxColour color = rule->appearance.color;
	return wxString::Format("rgba(%d, %d, %d, %d)", color.Red(), color.Green(), color.Blue(), color.Alpha());
}

wxString VisualsPage::BuildImagePathLabel(const std::optional<VisualRule>& rule) {
	if (!rule.has_value()) {
		return "N/A";
	}

	switch (rule->appearance.type) {
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			return rule->appearance.asset_path.empty() ? wxString("N/A") : wxString::FromUTF8(rule->appearance.asset_path);
		case VisualAppearanceType::OtherItemVisual:
		case VisualAppearanceType::SpriteId:
		case VisualAppearanceType::Rgba:
			return "N/A";
	}
	return "N/A";
}

void VisualsPage::ApplyDraftRules() {
	for (const auto& [_, rule] : draft_rules_) {
		working_copy_.SetUserRule(rule);
	}
	draft_rules_.clear();
}
