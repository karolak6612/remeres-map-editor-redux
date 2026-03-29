#ifndef RME_PREFERENCES_VISUALS_COLOR_DIALOG_H_
#define RME_PREFERENCES_VISUALS_COLOR_DIALOG_H_

#include <wx/dialog.h>

class wxColourPickerCtrl;
class wxSpinCtrl;

class ColorAlphaDialog final : public wxDialog {
public:
	ColorAlphaDialog(wxWindow* parent, const wxColour& initial_color);

	wxColour GetColour() const;

private:
	wxColourPickerCtrl* color_picker_ = nullptr;
	wxSpinCtrl* alpha_spin_ = nullptr;
};

#endif
