#ifndef RME_UI_MAP_EXPORT_MINIMAP_WINDOW_H_
#define RME_UI_MAP_EXPORT_MINIMAP_WINDOW_H_

#include "app/main.h"
#include "editor/persistence/minimap_exporter.h"

#include <array>
#include <vector>
#include <wx/dialog.h>

class Editor;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxNotebook;
class wxSizer;
class wxStaticText;
class wxTextCtrl;

class ExportMinimapWindow : public wxDialog {
public:
	ExportMinimapWindow(wxWindow* parent, Editor& editor);
	~ExportMinimapWindow() override;

	void OnClickBrowse(wxCommandEvent& event);
	void OnTextChanged(wxCommandEvent& event);
	void OnChoiceChanged(wxCommandEvent& event);
	void OnClickOK(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

private:
	struct FormatControls {
		wxChoice* image_size_choice = nullptr;
		std::vector<int> image_sizes;
		wxCheckBox* show_all_floors_checkbox = nullptr;
		wxCheckBox* shade_adjacent_floors_checkbox = nullptr;
	};

	void CheckValues();
	void UpdateControlState();
	wxPanel* CreateFormatPage(wxNotebook* notebook, MinimapExportFormat format);
	wxSizer* CreateAreaSizer();
	[[nodiscard]] FormatControls& CurrentControls();
	[[nodiscard]] const FormatControls& CurrentControls() const;

	[[nodiscard]] MinimapExportFormat SelectedFormat() const;
	[[nodiscard]] int SelectedImageSize() const;
	[[nodiscard]] MinimapExportFloorMask SelectedFloors() const;
	[[nodiscard]] std::string FileBaseName() const;

	Editor& editor_;

	wxStaticText* error_field_ = nullptr;
	wxTextCtrl* directory_text_field_ = nullptr;
	wxTextCtrl* file_name_text_field_ = nullptr;
	wxNotebook* notebook_ = nullptr;
	std::array<FormatControls, 3> format_controls_;
	std::array<wxCheckBox*, MAP_LAYERS> floor_checkboxes_ {};
	wxButton* ok_button_ = nullptr;
};

#endif
