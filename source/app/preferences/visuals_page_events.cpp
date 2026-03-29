#include "app/preferences/visuals_page.h"

#include "app/preferences/visuals_color_dialog.h"
#include "app/preferences/visuals_page_helpers.h"

#include "ui/find_item_window.h"

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

using namespace VisualsPageHelpers;

void VisualsPage::OnTreeSelected(wxTreeEvent& event) {
	if (shutting_down_ || suppress_events_ || !tree_ctrl_) {
		return;
	}

	ApplyCurrentEdits();

	selection_mode_ = SelectionMode::None;
	selected_key_.clear();
	selected_group_.clear();
	draft_rule_.reset();

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
		}
	}

	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnSearchChanged(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	ApplyCurrentEdits();
	PopulateTree();
	RefreshEditor();
	RefreshPreview();
}

void VisualsPage::OnAddItemOverride(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	FindItemDialog dialog(this, "Choose Item Visual");
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const uint16_t item_id = dialog.getResultID();
	if (item_id == 0) {
		wxMessageBox("Select a valid item.", "Visual Overrides", wxOK | wxICON_WARNING, this);
		return;
	}

	VisualRule rule = Visuals::MakeItemRule(item_id);
	working_copy_.SetUserRule(rule);
	PopulateTree();
	SelectRuleByKey(rule.key);
}

void VisualsPage::OnPickSprite(wxCommandEvent&) {
	if (shutting_down_ || selection_mode_ != SelectionMode::Rule) {
		return;
	}

	FindItemDialog dialog(this, "Choose Sprite Item");
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const uint16_t item_id = dialog.getResultID();
	if (item_id == 0) {
		return;
	}

	draft_rule_ = BuildEditableRule();
	draft_rule_->appearance.type = VisualAppearanceType::OtherItemVisual;
	draft_rule_->appearance.item_id = item_id;
	draft_rule_->appearance.asset_path.clear();
	draft_rule_->appearance.color = wxColour(255, 255, 255, 255);
	RefreshSelectedRuleState();
	RefreshPreview();
}

void VisualsPage::OnPickImage(wxCommandEvent&) {
	if (shutting_down_ || selection_mode_ != SelectionMode::Rule) {
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

	draft_rule_ = BuildEditableRule();
	const wxString path = dialog.GetPath();
	const wxString extension = wxFileName(path).GetExt().Lower();
	draft_rule_->appearance.type = extension == "png" ? VisualAppearanceType::Png : VisualAppearanceType::Svg;
	draft_rule_->appearance.asset_path = path.ToStdString();
	if (!draft_rule_->appearance.color.IsOk()) {
		draft_rule_->appearance.color = wxColour(255, 255, 255, 255);
	}
	RefreshSelectedRuleState();
	RefreshPreview();
}

void VisualsPage::OnPickColor(wxCommandEvent&) {
	if (shutting_down_ || selection_mode_ != SelectionMode::Rule) {
		return;
	}

	const wxColour initial_color = draft_rule_.has_value() ? draft_rule_->appearance.color :
		(GetSelectedRule().has_value() ? GetSelectedRule()->appearance.color : wxColour(255, 255, 255, 255));
	ColorAlphaDialog dialog(this, initial_color);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	draft_rule_ = BuildEditableRule();
	const wxColour chosen = dialog.GetColour();
	if (draft_rule_->appearance.type == VisualAppearanceType::Png || draft_rule_->appearance.type == VisualAppearanceType::Svg) {
		draft_rule_->appearance.color = chosen;
	} else {
		draft_rule_->appearance.type = VisualAppearanceType::Rgba;
		draft_rule_->appearance.color = chosen;
		draft_rule_->appearance.item_id = 0;
		draft_rule_->appearance.asset_path.clear();
	}
	RefreshSelectedRuleState();
	RefreshPreview();
}

void VisualsPage::OnResetSelected(wxCommandEvent&) {
	if (shutting_down_) {
		return;
	}
	switch (selection_mode_) {
		case SelectionMode::Rule:
			if (!selected_key_.empty()) {
				const std::string key = selected_key_;
				draft_rule_.reset();
				selected_key_.clear();
				selected_group_.clear();
				selection_mode_ = SelectionMode::None;
				working_copy_.RemoveUserRule(key);
				PopulateTree();
				SelectRuleByKey(key);
				RefreshEditor();
				RefreshPreview();
			}
			break;

		case SelectionMode::Group:
			draft_rule_.reset();
			selected_key_.clear();
			selection_mode_ = SelectionMode::Group;
			RemoveGroupOverrides(selected_group_);
			PopulateTree();
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
	draft_rule_.reset();
	selected_key_.clear();
	selected_group_.clear();
	selection_mode_ = SelectionMode::None;
	working_copy_.ClearUserOverrides();
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
