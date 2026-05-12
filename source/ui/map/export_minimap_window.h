#ifndef RME_UI_MAP_EXPORT_MINIMAP_WINDOW_H_
#define RME_UI_MAP_EXPORT_MINIMAP_WINDOW_H_

#include "app/main.h"
#include "editor/persistence/minimap_exporter.h"

#include <wx/dialog.h>

class Editor;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
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
	void CheckValues();
	void UpdateControlState();

	[[nodiscard]] MinimapExportFormat SelectedFormat() const;
	[[nodiscard]] MinimapExportFloorMode SelectedFloorMode() const;
	[[nodiscard]] int SelectedImageSize() const;
	[[nodiscard]] std::string FileBaseName() const;

	Editor& editor_;

	wxStaticText* error_field_ = nullptr;
	wxTextCtrl* directory_text_field_ = nullptr;
	wxTextCtrl* file_name_text_field_ = nullptr;
	wxChoice* format_choice_ = nullptr;
	wxChoice* image_size_choice_ = nullptr;
	wxChoice* floor_mode_choice_ = nullptr;
	wxSpinCtrl* floor_spin_ = nullptr;
	wxCheckBox* show_all_floors_checkbox_ = nullptr;
	wxButton* ok_button_ = nullptr;
};

#endif
