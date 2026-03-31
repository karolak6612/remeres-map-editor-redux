#include "app/preferences/visuals_page.h"

#include "app/preferences/preferences_layout.h"
#include "app/preferences/visuals_page_helpers.h"
#include "app/preferences/visuals_page_preview.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <ranges>

#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/statline.h>
#include <wx/stattext.h>

using namespace VisualsPageHelpers;

namespace {

constexpr std::array<const char*, 5> TabTitles {
	"Items",
	"Markers",
	"Overlays",
	"Tile & Zone",
	"Custom Items",
};

wxBitmapButton* makeIconButton(wxWindow* parent, std::string_view icon_path) {
	auto* button = new wxBitmapButton(parent, wxID_ANY, IMAGE_MANAGER.GetBitmap(icon_path, wxSize(18, 18)));
	button->SetMinSize(wxSize(32, 32));
	button->SetBitmapDisabled(IMAGE_MANAGER.GetBitmap(icon_path, wxSize(18, 18), wxColour(180, 180, 180)));
	return button;
}

wxStaticText* makeSectionLabel(wxWindow* parent, const std::string& text) {
	auto* label = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(text));
	wxFont font = label->GetFont();
	font.MakeBold();
	label->SetFont(font);
	label->SetForegroundColour(wxColour(84, 84, 84));
	return label;
}

}

VisualsPage::VisualsPage(wxWindow* parent) : PreferencesPage(parent), working_copy_(g_visuals) {
	BuildLayout();
	RebuildTabs();
}

VisualsPage::~VisualsPage() {
	BeginShutdown();
}

void VisualsPage::BuildLayout() {
	auto* root = new wxBoxSizer(wxVERTICAL);

	auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
	add_button_ = new wxButton(this, wxID_ADD, "New Item Visual...");
	import_button_ = new wxButton(this, wxID_OPEN, "Import...");
	export_button_ = new wxButton(this, wxID_SAVEAS, "Export...");
	reset_all_button_ = new wxButton(this, wxID_CLEAR, "Reset All");
	search_ctrl_ = new wxSearchCtrl(this, wxID_ANY);
	search_ctrl_->ShowSearchButton(true);
	search_ctrl_->ShowCancelButton(true);
	search_ctrl_->SetDescriptiveText("Search visuals");

	toolbar->Add(add_button_, 0, wxRIGHT, FromDIP(8));
	toolbar->Add(import_button_, 0, wxRIGHT, FromDIP(8));
	toolbar->Add(export_button_, 0, wxRIGHT, FromDIP(12));
	toolbar->Add(search_ctrl_, 1, wxRIGHT, FromDIP(12));
	toolbar->Add(reset_all_button_, 0);
	root->Add(toolbar, 0, wxEXPAND | wxALL, FromDIP(10));

	notebook_ = new wxNotebook(this, wxID_ANY);
	for (size_t index = 0; index < tabs_.size(); ++index) {
		auto* window = new wxScrolledWindow(notebook_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
		window->SetScrollRate(0, FromDIP(12));
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		window->SetSizer(sizer);
		tabs_[index] = TabWidgets { .window = window, .content = sizer };
		notebook_->AddPage(window, TabTitles[index]);
	}
	root->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
	SetSizer(root);

	search_ctrl_->Bind(wxEVT_TEXT, &VisualsPage::OnSearchChanged, this);
	add_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnAddItemOverride, this);
	import_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnImport, this);
	export_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnExport, this);
	reset_all_button_->Bind(wxEVT_BUTTON, &VisualsPage::OnResetAll, this);
	Bind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);
}

void VisualsPage::Apply() {
	if (shutting_down_) {
		return;
	}

	ApplyDraftRules();
	g_visuals = working_copy_;
	g_visuals.SaveUserOverrides();
	g_visuals.PrepareRuntimeResources();
	g_gui.RefreshView();
	RebuildTabs();
}

void VisualsPage::FocusContext(const VisualEditContext& context) {
	if (shutting_down_ || context.seed_rule.key.empty()) {
		return;
	}

	if (!working_copy_.GetEffectiveRule(context.seed_rule.key)) {
		working_copy_.SetUserRule(context.seed_rule);
	}

	focus_key_ = context.seed_rule.key;
	RebuildTabs();
}

void VisualsPage::RebuildTabs() {
	if (shutting_down_) {
		return;
	}

	row_widgets_.clear();
	const auto entries = BuildDisplayEntries();
	for (size_t index = 0; index < tabs_.size(); ++index) {
		tabs_[index].window->Freeze();
		tabs_[index].window->DestroyChildren();
		tabs_[index].content->Clear(false);
		BuildTab(static_cast<TabId>(index), entries);
		tabs_[index].window->Layout();
		tabs_[index].window->FitInside();
		tabs_[index].window->Thaw();
	}

	if (!focus_key_.empty()) {
		FocusRow(focus_key_);
		focus_key_.clear();
	}
}

void VisualsPage::BuildTab(TabId tab_id, const std::vector<DisplayEntry>& entries) {
	auto& tab = tabs_[static_cast<size_t>(tab_id)];
	AddHeaderRow(tab.window, tab.content);

	std::string current_section;
	bool added_row = false;
	for (const auto& entry : entries) {
		if (entry.tab != tab_id) {
			continue;
		}

		if (entry.section != current_section) {
			current_section = entry.section;
			AddSectionHeader(tab.window, tab.content, current_section);
		}
		AddVisualRow(tab.window, tab.content, entry);
		added_row = true;
	}

	if (!added_row) {
		tab.content->Add(new wxStaticText(tab.window, wxID_ANY, "No visuals in this tab."), 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(12));
	}
	tab.content->AddStretchSpacer();
}

void VisualsPage::AddHeaderRow(wxWindow* parent, wxSizer* sizer) {
	auto* row = new wxBoxSizer(wxHORIZONTAL);
	row->AddSpacer(FromDIP(220));
	row->Add(new wxStaticText(parent, wxID_ANY, "CURRENT"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(20));
	row->Add(new wxStaticText(parent, wxID_ANY, "PREVIEW"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(20));
	row->AddStretchSpacer();
	sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, FromDIP(8));
	sizer->Add(new wxStaticLine(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
}

void VisualsPage::AddSectionHeader(wxWindow* parent, wxSizer* sizer, const std::string& title) {
	sizer->AddSpacer(FromDIP(4));
	sizer->Add(makeSectionLabel(parent, title), 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));
}

void VisualsPage::AddVisualRow(wxWindow* parent, wxSizer* sizer, const DisplayEntry& entry) {
	auto* panel = new wxPanel(parent);
	auto* row = new wxBoxSizer(wxHORIZONTAL);

	auto* name_column = new wxBoxSizer(wxVERTICAL);
	auto* name_label = new wxStaticText(panel, wxID_ANY, wxString::FromUTF8(entry.seed_rule.label));
	auto* badge_label = new wxStaticText(panel, wxID_ANY, {});
	name_column->Add(name_label, 0, wxBOTTOM, FromDIP(2));
	name_column->Add(badge_label, 0);

	auto* current_preview = new VisualsPreviewPanel(panel, 32);
	auto* preview = new VisualsPreviewPanel(panel, 32);
	auto* sprite_button = makeIconButton(panel, ICON_CUBE);
	auto* svg_button = makeIconButton(panel, ICON_IMAGE_SOLID);
	auto* color_button = makeIconButton(panel, ICON_PALETTE);
	auto* reset_button = makeIconButton(panel, ICON_TRASH_CAN_SOLID);

	row->Add(name_column, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
	row->Add(current_preview, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
	row->Add(preview, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
	row->Add(sprite_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
	row->Add(svg_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
	row->Add(color_button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
	row->Add(reset_button, 0, wxALIGN_CENTER_VERTICAL);
	panel->SetSizer(row);

	sprite_button->Bind(wxEVT_BUTTON, [this, key = entry.key](wxCommandEvent&) { PickSpriteFor(key); });
	svg_button->Bind(wxEVT_BUTTON, [this, key = entry.key](wxCommandEvent&) { PickImageFor(key); });
	color_button->Bind(wxEVT_BUTTON, [this, key = entry.key](wxCommandEvent&) { PickColorFor(key); });
	reset_button->Bind(wxEVT_BUTTON, [this, key = entry.key](wxCommandEvent&) { ResetRow(key); });

	row_widgets_[entry.key] = RowWidgets {
		.panel = panel,
		.name_label = name_label,
		.badge_label = badge_label,
		.current_preview = current_preview,
		.preview = preview,
		.sprite_button = sprite_button,
		.svg_button = svg_button,
		.color_button = color_button,
		.reset_button = reset_button,
	};

	sizer->Add(panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
	RefreshRow(entry);
}

std::vector<VisualsPage::DisplayEntry> VisualsPage::BuildDisplayEntries() const {
	std::map<std::string, bool> keys;
	for (const auto& entry : g_visuals.BuildCatalog()) {
		if (const auto* rule = entry.effective()) {
			keys[rule->key] = true;
		}
	}
	for (const auto& entry : working_copy_.BuildCatalog()) {
		if (const auto* rule = entry.effective()) {
			keys[rule->key] = true;
		}
	}
	for (const auto& [key, _] : draft_rules_) {
		keys[key] = true;
	}

	std::vector<DisplayEntry> entries;
	for (const auto& [key, _] : keys) {
		const DisplayEntry entry = MakeDisplayEntry(key);
		if (!search_ctrl_ || search_ctrl_->GetValue().empty() || SearchTextFor(entry).find(LowercaseCopy(search_ctrl_->GetValue().ToStdString())) != std::string::npos) {
			entries.push_back(entry);
		}
	}

	std::ranges::sort(entries, [](const DisplayEntry& left, const DisplayEntry& right) {
		if (left.tab != right.tab) {
			return left.tab < right.tab;
		}
		if (left.section != right.section) {
			return left.section < right.section;
		}
		return left.seed_rule.label < right.seed_rule.label;
	});

	return entries;
}

std::optional<VisualRule> VisualsPage::RuleForKey(const Visuals& visuals, const std::string& key) const {
	if (const auto* rule = visuals.GetEffectiveRule(key)) {
		return *rule;
	}

	if (const auto item_id = ParseItemId(key); item_id.has_value()) {
		return Visuals::MakeItemRule(*item_id);
	}
	return std::nullopt;
}

std::optional<uint16_t> VisualsPage::ParseItemId(const std::string& key) const {
	static constexpr std::string_view Prefix = "item.id.";
	if (!key.starts_with(Prefix)) {
		return std::nullopt;
	}

	try {
		return static_cast<uint16_t>(std::stoi(key.substr(Prefix.size())));
	} catch (...) {
		return std::nullopt;
	}
}

VisualRule VisualsPage::NormalizeRule(VisualRule rule) const {
	Visuals scratch = working_copy_;
	scratch.SetUserRule(rule);
	if (const auto* normalized = scratch.GetUserRule(rule.key.empty() ? Visuals::MakeKeyForItemId(static_cast<uint16_t>(rule.match_id)) : rule.key)) {
		return *normalized;
	}
	return rule;
}

VisualsPage::DisplayEntry VisualsPage::MakeDisplayEntry(const std::string& key) const {
	DisplayEntry entry;
	entry.key = key;
	entry.current_rule = RuleForKey(g_visuals, key);
	entry.preview_rule = draft_rules_.contains(key) ? std::optional<VisualRule>(draft_rules_.at(key)) : RuleForKey(working_copy_, key);
	entry.current_has_override = g_visuals.GetUserRule(key) != nullptr;
	entry.preview_has_override = working_copy_.GetUserRule(key) != nullptr || draft_rules_.contains(key);
	const bool preview_is_fallback_item = !working_copy_.GetEffectiveRule(key) && ParseItemId(key).has_value();
	entry.seed_rule = preview_is_fallback_item && entry.current_rule ? *entry.current_rule :
		entry.preview_rule ? *entry.preview_rule :
		entry.current_rule ? *entry.current_rule :
		VisualRule { .key = key };
	entry.tab = ClassifyTab(entry.seed_rule, entry.preview_has_override || entry.current_has_override);
	entry.section = ClassifySection(entry.seed_rule, entry.tab, entry.preview_has_override || entry.current_has_override);
	return entry;
}

VisualsPage::TabId VisualsPage::ClassifyTab(const VisualRule& seed_rule, bool preview_has_override) const {
	switch (seed_rule.match_type) {
		case VisualMatchType::ItemId:
			return preview_has_override && !working_copy_.GetDefaultRule(seed_rule.key) ? TabId::CustomItems : TabId::Items;
		case VisualMatchType::Marker:
			return TabId::Markers;
		case VisualMatchType::Overlay:
			return TabId::Overlays;
		case VisualMatchType::Tile:
			return TabId::TileZone;
		case VisualMatchType::ClientId:
			return TabId::Items;
	}
	return TabId::Items;
}

std::string VisualsPage::ClassifySection(const VisualRule& seed_rule, TabId tab_id, bool) const {
	if (tab_id == TabId::Items) {
		if (seed_rule.label.contains("Primal")) {
			return "Primal";
		}
		if (seed_rule.label.contains("Invisible")) {
			return "Technical";
		}
		return "Defaults";
	}
	if (tab_id == TabId::Markers) {
		return "Map Markers";
	}
	if (tab_id == TabId::Overlays) {
		return "Indicators";
	}
	if (tab_id == TabId::TileZone) {
		if (seed_rule.match_value == "house_overlay") {
			return "House";
		}
		return "Zone & Tile";
	}
	return "Custom Item Overrides";
}

std::string VisualsPage::SearchTextFor(const DisplayEntry& entry) const {
	return LowercaseCopy(entry.seed_rule.label + " " + entry.seed_rule.key + " " + entry.section);
}

void VisualsPage::RefreshAllRows() {
	for (const auto& entry : BuildDisplayEntries()) {
		RefreshRow(entry);
	}
}

void VisualsPage::RefreshRow(const DisplayEntry& entry) {
	const auto iterator = row_widgets_.find(entry.key);
	if (iterator == row_widgets_.end()) {
		return;
	}

	auto& row = iterator->second;
	row.name_label->SetLabel(wxString::FromUTF8(entry.seed_rule.label));
	row.badge_label->SetLabel(BuildBadgeLabel(entry.preview_has_override || entry.current_has_override, entry.preview_rule.has_value() && !entry.preview_rule->valid));
	StyleBadge(row.badge_label, entry.preview_rule.has_value() && !entry.preview_rule->valid);
	row.current_preview->SetRule(entry.current_rule);
	row.preview->SetRule(entry.preview_rule);
	RefreshRowButtons(entry.key, entry.seed_rule);
	row.panel->Layout();
}

void VisualsPage::RefreshRowButtons(const std::string& key, const VisualRule& seed_rule) {
	const auto iterator = row_widgets_.find(key);
	if (iterator == row_widgets_.end()) {
		return;
	}

	const bool has_draft = draft_rules_.contains(key);
	const auto draft = has_draft ? std::optional<VisualRule>(draft_rules_.at(key)) : std::nullopt;
	const bool supports_sprite = Visuals::SupportsSpriteModes(seed_rule);
	const bool supports_image = Visuals::SupportsImageModes(seed_rule);

	bool sprite_enabled = supports_sprite;
	bool svg_enabled = supports_image;
	bool color_enabled = true;
	bool sprite_active = false;
	bool svg_active = false;
	bool color_active = false;

	if (draft.has_value()) {
		switch (draft->appearance.type) {
			case VisualAppearanceType::OtherItemVisual:
			case VisualAppearanceType::SpriteId:
				sprite_enabled = true;
				svg_enabled = false;
				color_enabled = false;
				sprite_active = true;
				break;
			case VisualAppearanceType::Png:
				sprite_enabled = false;
				svg_enabled = true;
				color_enabled = false;
				svg_active = true;
				break;
			case VisualAppearanceType::Svg:
				sprite_enabled = false;
				svg_enabled = true;
				color_enabled = true;
				svg_active = true;
				color_active = !IsNeutralColor(draft->appearance.color);
				break;
			case VisualAppearanceType::Rgba:
				sprite_enabled = false;
				svg_enabled = false;
				color_enabled = true;
				color_active = true;
				break;
		}
	}

	StyleActionButton(iterator->second.sprite_button, sprite_enabled, sprite_active);
	StyleActionButton(iterator->second.svg_button, svg_enabled, svg_active);
	StyleActionButton(iterator->second.color_button, color_enabled, color_active);
	StyleActionButton(iterator->second.reset_button, working_copy_.GetUserRule(key) != nullptr || draft_rules_.contains(key) || g_visuals.GetUserRule(key) != nullptr, false);
}

void VisualsPage::FocusRow(const std::string& key) {
	const DisplayEntry entry = MakeDisplayEntry(key);
	notebook_->SetSelection(static_cast<int>(entry.tab));
	if (const auto iterator = row_widgets_.find(key); iterator != row_widgets_.end()) {
		const int y_pixels = iterator->second.panel->GetPosition().y;
		tabs_[static_cast<size_t>(entry.tab)].window->Scroll(0, y_pixels / FromDIP(12));
	}
}

void VisualsPage::ApplyDraftRules() {
	for (const auto& [_, rule] : draft_rules_) {
		working_copy_.SetUserRule(rule);
	}
	draft_rules_.clear();
}
