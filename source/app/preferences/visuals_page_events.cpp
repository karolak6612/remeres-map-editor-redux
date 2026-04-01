#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_color_dialog.h"
#include "ui/find_item_window.h"

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>
#include <wx/slider.h>
#include <wx/treelist.h>

void VisualsPage::PickSpriteFor(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	FindItemDialog dialog(this, "Choose Sprite Item");
	if (dialog.ShowModal() != wxID_OK || dialog.getResultID() == 0) {
		return;
	}

	const auto preview_rule = draft_rules_.contains(key) ? std::optional<VisualRule>(draft_rules_.at(key)) : RuleForKey(working_copy_, key);
	VisualRule rule = preview_rule.value_or(ParseItemId(key).has_value() ? Visuals::MakeItemRule(*ParseItemId(key)) : VisualRule {});
	rule.key = key;
	rule.appearance.type = VisualAppearanceType::OtherItemVisual;
	rule.appearance.item_id = dialog.getResultID();
	rule.appearance.sprite_id = 0;
	rule.appearance.asset_path.clear();
	rule.appearance.color = wxColour(255, 255, 255, 255);
	draft_rules_[key] = NormalizeRule(std::move(rule));
	FocusRow(key);
}

void VisualsPage::PickImageFor(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	wxFileDialog dialog(
		this,
		"Choose SVG / PNG",
		wxEmptyString,
		wxEmptyString,
		"SVG and PNG files (*.svg;*.png)|*.svg;*.png|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST
	);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const auto preview_rule = draft_rules_.contains(key) ? std::optional<VisualRule>(draft_rules_.at(key)) : RuleForKey(working_copy_, key);
	VisualRule rule = preview_rule.value_or(ParseItemId(key).has_value() ? Visuals::MakeItemRule(*ParseItemId(key)) : VisualRule {});
	rule.key = key;
	rule.appearance.type = wxFileName(dialog.GetPath()).GetExt().Lower() == "png" ? VisualAppearanceType::Png : VisualAppearanceType::Svg;
	rule.appearance.asset_path = dialog.GetPath().ToStdString();
	rule.appearance.item_id = 0;
	rule.appearance.sprite_id = 0;
	if (!rule.appearance.color.IsOk()) {
		rule.appearance.color = wxColour(255, 255, 255, 255);
	}
	draft_rules_[key] = NormalizeRule(std::move(rule));
	FocusRow(key);
}

void VisualsPage::PickColorFor(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	const auto preview_rule = draft_rules_.contains(key) ? std::optional<VisualRule>(draft_rules_.at(key)) : RuleForKey(working_copy_, key);
	const wxColour initial_color = preview_rule.has_value() ? preview_rule->appearance.color : wxColour(255, 255, 255, 255);
	ColorAlphaDialog dialog(this, initial_color);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	VisualRule rule = preview_rule.value_or(ParseItemId(key).has_value() ? Visuals::MakeItemRule(*ParseItemId(key)) : VisualRule {});
	rule.key = key;
	if (rule.appearance.type == VisualAppearanceType::Svg) {
		rule.appearance.color = dialog.GetColour();
	} else {
		rule.appearance.type = VisualAppearanceType::Rgba;
		rule.appearance.color = dialog.GetColour();
		rule.appearance.item_id = 0;
		rule.appearance.sprite_id = 0;
		rule.appearance.asset_path.clear();
	}
	draft_rules_[key] = NormalizeRule(std::move(rule));
	FocusRow(key);
}

void VisualsPage::ResetRow(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	draft_rules_.erase(key);
	working_copy_.RemoveUserRule(key);
	focus_key_ = key;
	RebuildTree();
}

void VisualsPage::OnSearchChanged(wxCommandEvent&) {
	RebuildTree();
}

void VisualsPage::PickSpriteForSelected() {
	if (!selected_key_.empty()) {
		PickSpriteFor(selected_key_);
	}
}

void VisualsPage::PickImageForSelected() {
	if (!selected_key_.empty()) {
		PickImageFor(selected_key_);
	}
}

void VisualsPage::PickColorForSelected() {
	if (!selected_key_.empty()) {
		PickColorFor(selected_key_);
	}
}

void VisualsPage::ResetSelected() {
	if (selected_key_.empty()) {
		return;
	}

	const auto answer = wxMessageBox("Remove this visual override and restore the default?", "Remove Visual Override", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this);
	if (answer == wxYES) {
		ResetRow(selected_key_);
	}
}

void VisualsPage::CommitNameChange() {
	if (shutting_down_ || selected_key_.empty() || !detail_.name_ctrl) {
		return;
	}

	const auto preview_rule = draft_rules_.contains(selected_key_) ? std::optional<VisualRule>(draft_rules_.at(selected_key_)) : RuleForKey(working_copy_, selected_key_);
	if (!preview_rule.has_value()) {
		return;
	}

	VisualRule rule = *preview_rule;
	const wxString entered = detail_.name_ctrl->GetValue().Trim(true).Trim(false);
	rule.label = entered.empty() ? MakeDisplayEntry(selected_key_).seed_rule.label : entered.ToStdString();
	draft_rules_[selected_key_] = NormalizeRule(std::move(rule));
	focus_key_ = selected_key_;
	RebuildTree();
}

void VisualsPage::ApplyAlphaForSelected(int alpha) {
	if (shutting_down_ || selected_key_.empty()) {
		return;
	}

	const auto preview_rule = draft_rules_.contains(selected_key_) ? std::optional<VisualRule>(draft_rules_.at(selected_key_)) : RuleForKey(working_copy_, selected_key_);
	if (!preview_rule.has_value()) {
		return;
	}

	VisualRule rule = *preview_rule;
	if (rule.appearance.type != VisualAppearanceType::Rgba && rule.appearance.type != VisualAppearanceType::Svg) {
		return;
	}

	const wxColour current = rule.appearance.color.IsOk() ? rule.appearance.color : wxColour(255, 255, 255, 255);
	rule.appearance.color = wxColour(current.Red(), current.Green(), current.Blue(), static_cast<unsigned char>(alpha));
	draft_rules_[selected_key_] = NormalizeRule(std::move(rule));
	RefreshDetail();
}

void VisualsPage::OnNameCommit(wxCommandEvent& event) {
	CommitNameChange();
	event.Skip();
}

void VisualsPage::OnNameFocusLost(wxFocusEvent& event) {
	CommitNameChange();
	event.Skip();
}

void VisualsPage::OnAlphaChanged(wxCommandEvent& event) {
	ApplyAlphaForSelected(detail_.alpha_slider ? detail_.alpha_slider->GetValue() : 255);
	event.Skip();
}

void VisualsPage::OnAddItemOverride(wxCommandEvent&) {
	FindItemDialog dialog(this, "Choose Item Visual");
	if (dialog.ShowModal() != wxID_OK || dialog.getResultID() == 0) {
		return;
	}

	VisualRule rule = Visuals::MakeItemRule(dialog.getResultID());
	working_copy_.SetUserRule(rule);
	focus_key_ = rule.key;
	RebuildTree();
}

void VisualsPage::OnImport(wxCommandEvent&) {
	wxFileDialog dialog(this, "Import Visual Overrides", wxEmptyString, "visuals.toml", "TOML files (*.toml)|*.toml|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	if (!working_copy_.ImportUserOverrides(dialog.GetPath())) {
		wxMessageBox("Failed to import the selected TOML file.", "Visual Overrides", wxOK | wxICON_ERROR, this);
		return;
	}

	draft_rules_.clear();
	RebuildTree();
}

void VisualsPage::OnExport(wxCommandEvent&) {
	wxFileDialog dialog(this, "Export Visual Overrides", wxEmptyString, "visuals.toml", "TOML files (*.toml)|*.toml|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	ApplyDraftRules();
	if (!working_copy_.ExportUserOverrides(dialog.GetPath())) {
		wxMessageBox("Failed to export the selected TOML file.", "Visual Overrides", wxOK | wxICON_ERROR, this);
	}
	RebuildTree();
}

void VisualsPage::OnResetAll(wxCommandEvent&) {
	const auto answer = wxMessageBox(
		"Reset all visuals to defaults? This removes every custom visual override.",
		"Reset All Visual Overrides",
		wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
		this
	);
	if (answer != wxYES) {
		return;
	}

	draft_rules_.clear();
	working_copy_.ClearUserOverrides();
	RebuildTree();
}

void VisualsPage::OnWindowDestroy(wxWindowDestroyEvent& event) {
	if (event.GetEventObject() == this) {
		BeginShutdown();
	}
	event.Skip();
}

void VisualsPage::BeginShutdown() {
	if (shutting_down_) {
		return;
	}

	shutting_down_ = true;
	if (search_ctrl_) {
		search_ctrl_->Unbind(wxEVT_TEXT, &VisualsPage::OnSearchChanged, this);
	}
	if (tree_) {
		tree_->Unbind(wxEVT_TREELIST_SELECTION_CHANGED, &VisualsPage::OnTreeSelectionChanged, this);
	}
	if (add_button_) {
		add_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnAddItemOverride, this);
	}
	if (import_button_) {
		import_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnImport, this);
	}
	if (export_button_) {
		export_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnExport, this);
	}
	if (reset_all_button_) {
		reset_all_button_->Unbind(wxEVT_BUTTON, &VisualsPage::OnResetAll, this);
	}
	if (detail_.name_ctrl) {
		detail_.name_ctrl->Unbind(wxEVT_TEXT_ENTER, &VisualsPage::OnNameCommit, this);
		detail_.name_ctrl->Unbind(wxEVT_KILL_FOCUS, &VisualsPage::OnNameFocusLost, this);
	}
	if (detail_.alpha_slider) {
		detail_.alpha_slider->Unbind(wxEVT_SLIDER, &VisualsPage::OnAlphaChanged, this);
	}
	Unbind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);
}
