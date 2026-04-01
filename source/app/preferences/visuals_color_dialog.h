#ifndef RME_PREFERENCES_VISUALS_COLOR_DIALOG_H_
#define RME_PREFERENCES_VISUALS_COLOR_DIALOG_H_

#include <wx/dialog.h>

class wxBitmapButton;
class wxCommandEvent;
class wxPanel;
class wxSlider;
class wxStaticText;
class wxMouseEvent;

class ColorAlphaDialog final : public wxDialog {
public:
	ColorAlphaDialog(wxWindow* parent, const wxColour& initial_color);

	wxColour GetColour() const;

private:
	void OpenColorPicker();
	void UpdateSwatch();
	void UpdateAlphaLabel();
	void OnAlphaChanged(wxCommandEvent& event);
	void OnResetAlpha(wxCommandEvent& event);
	void OnSwatchClicked(wxMouseEvent& event);

	wxPanel* swatch_panel_ = nullptr;
	wxSlider* alpha_slider_ = nullptr;
	wxStaticText* alpha_value_label_ = nullptr;
	wxBitmapButton* reset_alpha_button_ = nullptr;
	wxColour color_;
};

#endif
