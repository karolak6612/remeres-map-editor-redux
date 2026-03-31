#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_color_dialog.h"
#include "ui/find_item_window.h"

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>

void VisualsPage::PickSpriteFor(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	FindItemDialog dialog(this, "Choose Sprite Item");
	if (dialog.ShowModal() != wxID_OK || dialog.getResultID() == 0) {
		return;
	}

	VisualRule rule = RuleForKey(working_copy_, key).value_or(ParseItemId(key).has_value() ? Visuals::MakeItemRule(*ParseItemId(key)) : VisualRule {});
	rule.key = key;
	rule.appearance.type = VisualAppearanceType::OtherItemVisual;
	rule.appearance.item_id = dialog.getResultID();
	rule.appearance.sprite_id = 0;
	rule.appearance.asset_path.clear();
	rule.appearance.color = wxColour(255, 255, 255, 255);
	draft_rules_[key] = NormalizeRule(std::move(rule));
	RefreshRow(MakeDisplayEntry(key));
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

	VisualRule rule = RuleForKey(working_copy_, key).value_or(ParseItemId(key).has_value() ? Visuals::MakeItemRule(*ParseItemId(key)) : VisualRule {});
	rule.key = key;
	rule.appearance.type = wxFileName(dialog.GetPath()).GetExt().Lower() == "png" ? VisualAppearanceType::Png : VisualAppearanceType::Svg;
	rule.appearance.asset_path = dialog.GetPath().ToStdString();
	rule.appearance.item_id = 0;
	rule.appearance.sprite_id = 0;
	if (!rule.appearance.color.IsOk()) {
		rule.appearance.color = wxColour(255, 255, 255, 255);
	}
	draft_rules_[key] = NormalizeRule(std::move(rule));
	RefreshRow(MakeDisplayEntry(key));
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
	RefreshRow(MakeDisplayEntry(key));
	FocusRow(key);
}

void VisualsPage::ResetRow(const std::string& key) {
	if (shutting_down_) {
		return;
	}

	draft_rules_.erase(key);
	working_copy_.RemoveUserRule(key);
	focus_key_ = key;
	RebuildTabs();
}

void VisualsPage::OnSearchChanged(wxCommandEvent&) {
	RebuildTabs();
}

void VisualsPage::OnAddItemOverride(wxCommandEvent&) {
	FindItemDialog dialog(this, "Choose Item Visual");
	if (dialog.ShowModal() != wxID_OK || dialog.getResultID() == 0) {
		return;
	}

	VisualRule rule = Visuals::MakeItemRule(dialog.getResultID());
	working_copy_.SetUserRule(rule);
	focus_key_ = rule.key;
	RebuildTabs();
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
	RebuildTabs();
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
	RebuildTabs();
}

void VisualsPage::OnResetAll(wxCommandEvent&) {
	draft_rules_.clear();
	working_copy_.ClearUserOverrides();
	RebuildTabs();
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
	Unbind(wxEVT_DESTROY, &VisualsPage::OnWindowDestroy, this);
}
