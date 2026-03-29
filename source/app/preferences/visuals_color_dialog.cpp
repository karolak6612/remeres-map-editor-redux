#include "app/preferences/visuals_color_dialog.h"

#include <wx/clrpicker.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

ColorAlphaDialog::ColorAlphaDialog(wxWindow* parent, const wxColour& initial_color) :
	wxDialog(parent, wxID_ANY, "Pick Color", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
	auto* root = new wxBoxSizer(wxVERTICAL);

	auto* picker_row = new wxBoxSizer(wxHORIZONTAL);
	picker_row->Add(new wxStaticText(this, wxID_ANY, "Color"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
	color_picker_ = new wxColourPickerCtrl(
		this,
		wxID_ANY,
		wxColour(initial_color.Red(), initial_color.Green(), initial_color.Blue(), 255)
	);
	picker_row->Add(color_picker_, 1, wxEXPAND);
	root->Add(picker_row, 0, wxEXPAND | wxALL, FromDIP(10));

	auto* alpha_row = new wxBoxSizer(wxHORIZONTAL);
	alpha_row->Add(new wxStaticText(this, wxID_ANY, "Alpha"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
	alpha_spin_ = new wxSpinCtrl(this, wxID_ANY);
	alpha_spin_->SetRange(0, 255);
	alpha_spin_->SetValue(initial_color.IsOk() ? initial_color.Alpha() : 255);
	alpha_row->Add(alpha_spin_, 0);
	root->Add(alpha_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));

	root->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));

	SetSizerAndFit(root);
	SetMinSize(FromDIP(wxSize(280, 0)));
}

wxColour ColorAlphaDialog::GetColour() const {
	const wxColour color = color_picker_ ? color_picker_->GetColour() : wxColour(255, 255, 255, 255);
	const int alpha = alpha_spin_ ? alpha_spin_->GetValue() : 255;
	return wxColour(color.Red(), color.Green(), color.Blue(), static_cast<unsigned char>(alpha));
}
