#include "ui/map/export_minimap_window.h"

#include "app/application.h"
#include "editor/editor.h"
#include "ui/dialog_util.h"
#include "ui/gui.h"
#include "ui/gui_ids.h"
#include "util/image_manager.h"

#include <algorithm>
#include <array>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dirdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace {

constexpr std::array<int, 4> kImageSizes { 512, 1024, 2048, 4096 };

[[nodiscard]] wxString defaultFileName(const Editor& editor) {
	const wxString mapName = wxString::FromUTF8(editor.map.getName().c_str());
	if (mapName.empty()) {
		return "minimap";
	}

	const wxFileName fileName(mapName);
	const wxString baseName = fileName.GetName();
	return baseName.empty() ? mapName : baseName;
}

[[nodiscard]] bool hasInvalidFileNameChars(const wxString& value) {
	const std::string fileName = value.ToStdString();
	return fileName.find_first_of("<>:\"/\\|?*") != std::string::npos;
}

[[nodiscard]] int clampedCurrentFloor() {
	return std::clamp(g_gui.GetCurrentFloor(), 0, MAP_MAX_LAYER);
}

} // namespace

ExportMinimapWindow::ExportMinimapWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export Minimap", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	editor_(editor) {
	auto* sizer = newd wxBoxSizer(wxVERTICAL);

	error_field_ = newd wxStaticText(this, wxID_VIEW_DETAILS, "", wxDefaultPosition, wxDefaultSize);
	error_field_->SetForegroundColour(*wxRED);
	auto* errorSizer = newd wxBoxSizer(wxHORIZONTAL);
	errorSizer->Add(error_field_, 0, wxALL, 5);
	sizer->Add(errorSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	directory_text_field_ = newd wxTextCtrl(this, wxID_ANY, wxString(g_settings.getString(Config::MINIMAP_EXPORT_DIR)), wxDefaultPosition, wxDefaultSize);
	directory_text_field_->Bind(wxEVT_TEXT, &ExportMinimapWindow::OnTextChanged, this);
	auto* folderSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Output Folder");
	folderSizer->Add(directory_text_field_, 1, wxALL | wxEXPAND, 5);
	auto* browseButton = newd wxButton(this, MAP_WINDOW_FILE_BUTTON, "Browse");
	browseButton->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_FOLDER_OPEN));
	folderSizer->Add(browseButton, 0, wxALL, 5);
	sizer->Add(folderSizer, 0, wxALL | wxEXPAND, 5);

	file_name_text_field_ = newd wxTextCtrl(this, wxID_ANY, defaultFileName(editor_), wxDefaultPosition, wxDefaultSize);
	file_name_text_field_->Bind(wxEVT_TEXT, &ExportMinimapWindow::OnTextChanged, this);
	auto* fileSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "File Name");
	fileSizer->Add(file_name_text_field_, 1, wxALL | wxEXPAND, 5);
	sizer->Add(fileSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	format_choice_ = newd wxChoice(this, wxID_ANY);
	format_choice_->Append(".otmm");
	format_choice_->Append(".jpg");
	format_choice_->Append(".webp");
	format_choice_->SetSelection(0);
	format_choice_->Bind(wxEVT_CHOICE, &ExportMinimapWindow::OnChoiceChanged, this);
	auto* formatSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Export Format");
	formatSizer->Add(format_choice_, 1, wxALL | wxEXPAND, 5);
	sizer->Add(formatSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	image_size_choice_ = newd wxChoice(this, wxID_ANY);
	for (const int size : kImageSizes) {
		image_size_choice_->Append(wxString::Format("%d x %d", size, size));
	}
	image_size_choice_->SetSelection(1);
	image_size_choice_->Bind(wxEVT_CHOICE, &ExportMinimapWindow::OnChoiceChanged, this);
	auto* imageSizeSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Image Size");
	imageSizeSizer->Add(image_size_choice_, 1, wxALL | wxEXPAND, 5);
	sizer->Add(imageSizeSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	floor_mode_choice_ = newd wxChoice(this, wxID_ANY);
	floor_mode_choice_->Append("All floors");
	floor_mode_choice_->Append("Ground floor");
	floor_mode_choice_->Append("Selected floor");
	floor_mode_choice_->SetSelection(0);
	floor_mode_choice_->Bind(wxEVT_CHOICE, &ExportMinimapWindow::OnChoiceChanged, this);
	floor_spin_ = newd wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FROM_DIP(this, wxSize(72, -1)), wxSP_ARROW_KEYS, 0, MAP_MAX_LAYER, clampedCurrentFloor());
	floor_spin_->SetMinSize(FROM_DIP(this, wxSize(72, -1)));
	auto* floorSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Area");
	floorSizer->Add(floor_mode_choice_, 1, wxALL | wxEXPAND, 5);
	floorSizer->Add(newd wxStaticText(this, wxID_ANY, "Floor"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	floorSizer->Add(floor_spin_, 0, wxALL, 5);
	sizer->Add(floorSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	show_all_floors_checkbox_ = newd wxCheckBox(this, wxID_ANY, "Enable show all floors during export");
	show_all_floors_checkbox_->SetValue(g_settings.getBoolean(Config::SHOW_ALL_FLOORS));
	sizer->Add(show_all_floors_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

	auto* buttonSizer = newd wxBoxSizer(wxHORIZONTAL);
	ok_button_ = newd wxButton(this, wxID_OK, "OK");
	ok_button_->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_CHECK));
	buttonSizer->Add(ok_button_, wxSizerFlags(1).Center());
	auto* cancelButton = newd wxButton(this, wxID_CANCEL, "Cancel");
	cancelButton->SetBitmap(IMAGE_MANAGER.GetBitmapBundle(ICON_XMARK));
	buttonSizer->Add(cancelButton, wxSizerFlags(1).Center());
	sizer->Add(buttonSizer, 0, wxCENTER, 10);

	SetSizerAndFit(sizer);
	SetMinSize(GetSize());
	Centre(wxBOTH);
	UpdateControlState();
	CheckValues();

	browseButton->Bind(wxEVT_BUTTON, &ExportMinimapWindow::OnClickBrowse, this);
	ok_button_->Bind(wxEVT_BUTTON, &ExportMinimapWindow::OnClickOK, this);
	cancelButton->Bind(wxEVT_BUTTON, &ExportMinimapWindow::OnClickCancel, this);
	SetIcons(IMAGE_MANAGER.GetIconBundle(ICON_FILE_EXPORT));
}

ExportMinimapWindow::~ExportMinimapWindow() = default;

void ExportMinimapWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event)) {
	wxDirDialog dialog(nullptr, "Select the output folder", directory_text_field_->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dialog.ShowModal() == wxID_OK) {
		directory_text_field_->ChangeValue(dialog.GetPath());
	}
	CheckValues();
}

void ExportMinimapWindow::OnTextChanged(wxCommandEvent& event) {
	CheckValues();
	event.Skip();
}

void ExportMinimapWindow::OnChoiceChanged(wxCommandEvent& event) {
	UpdateControlState();
	CheckValues();
	event.Skip();
}

void ExportMinimapWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	const wxFileName directory = wxFileName::DirName(directory_text_field_->GetValue());
	g_settings.setString(Config::MINIMAP_EXPORT_DIR, directory.GetFullPath().ToStdString());

	MinimapExportOptions options {
		.outputDirectory = directory,
		.fileBaseName = FileBaseName(),
		.format = SelectedFormat(),
		.floorMode = SelectedFloorMode(),
		.selectedFloor = floor_spin_->GetValue(),
		.imageSize = SelectedImageSize(),
		.showAllFloors = show_all_floors_checkbox_->GetValue(),
	};

	g_gui.CreateLoadBar("Exporting Minimap");
	const MinimapExportResult result = MinimapExporter::Export(editor_, options, [](uint64_t completed, uint64_t total) {
		if (total > 0) {
			g_gui.SetLoadDone(static_cast<int32_t>((completed * 100) / total));
		}
	});
	g_gui.DestroyLoadBar();

	if (!result.ok) {
		DialogUtil::PopupDialog(this, "Error", wxString::FromUTF8(result.error.c_str()), wxOK);
		return;
	}

	if (result.filesWritten == 0) {
		DialogUtil::PopupDialog(this, "Export Minimap", "No minimap tiles found to export.", wxOK);
	} else {
		DialogUtil::PopupDialog(this, "Export Minimap", wxString::Format("Exported %llu minimap file(s).", static_cast<unsigned long long>(result.filesWritten)), wxOK);
	}
	EndModal(wxID_OK);
}

void ExportMinimapWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}

void ExportMinimapWindow::CheckValues() {
	if (directory_text_field_->IsEmpty()) {
		error_field_->SetLabel("Type or select an output folder.");
		ok_button_->Enable(false);
		return;
	}
	if (file_name_text_field_->IsEmpty()) {
		error_field_->SetLabel("Type a name for the file.");
		ok_button_->Enable(false);
		return;
	}
	if (hasInvalidFileNameChars(file_name_text_field_->GetValue())) {
		error_field_->SetLabel("File name contains invalid characters.");
		ok_button_->Enable(false);
		return;
	}

	const wxFileName directory = wxFileName::DirName(directory_text_field_->GetValue());
	if (!directory.DirExists()) {
		error_field_->SetLabel("Output folder not found.");
		ok_button_->Enable(false);
		return;
	}
	if (!directory.IsDirWritable()) {
		error_field_->SetLabel("Output folder is not writable.");
		ok_button_->Enable(false);
		return;
	}

	error_field_->SetLabel(wxEmptyString);
	ok_button_->Enable(true);
}

void ExportMinimapWindow::UpdateControlState() {
	const bool isImage = SelectedFormat() != MinimapExportFormat::Otmm;
	const bool isSelectedFloor = SelectedFloorMode() == MinimapExportFloorMode::SelectedFloor;
	image_size_choice_->Enable(isImage);
	floor_spin_->Enable(isSelectedFloor);
	show_all_floors_checkbox_->Enable(isImage && SelectedFloorMode() != MinimapExportFloorMode::AllFloors);
}

MinimapExportFormat ExportMinimapWindow::SelectedFormat() const {
	return static_cast<MinimapExportFormat>(std::max(0, format_choice_->GetSelection()));
}

MinimapExportFloorMode ExportMinimapWindow::SelectedFloorMode() const {
	return static_cast<MinimapExportFloorMode>(std::max(0, floor_mode_choice_->GetSelection()));
}

int ExportMinimapWindow::SelectedImageSize() const {
	const int index = std::clamp(image_size_choice_->GetSelection(), 0, static_cast<int>(kImageSizes.size() - 1));
	return kImageSizes[static_cast<size_t>(index)];
}

std::string ExportMinimapWindow::FileBaseName() const {
	const wxString text = file_name_text_field_->GetValue();
	const wxFileName fileName(text);
	const wxString baseName = fileName.GetName();
	return (baseName.empty() ? text : baseName).ToStdString();
}
