#include "app/preferences/visuals_color_dialog.h"

#include "util/image_manager.h"

#include <algorithm>

#include <wx/bmpbuttn.h>
#include <wx/colordlg.h>
#include <wx/dcbuffer.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {

constexpr int DefaultAlpha = 171;

class ColorSwatchPanel final : public wxPanel {
public:
	explicit ColorSwatchPanel(wxWindow* parent) :
		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(48, 48), wxBORDER_SIMPLE) {
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		SetMinSize(wxSize(48, 48));
		Bind(wxEVT_PAINT, &ColorSwatchPanel::OnPaint, this);
	}

	void SetColour(const wxColour& color) {
		color_ = color;
		Refresh();
	}

private:
	void OnPaint(wxPaintEvent&) {
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(GetBackgroundColour()));
		dc.Clear();

		const wxRect rect = GetClientRect().Deflate(1);
		constexpr int Cell = 8;
		for (int y = rect.y; y < rect.GetBottom(); y += Cell) {
			for (int x = rect.x; x < rect.GetRight(); x += Cell) {
				const bool light = ((x - rect.x) / Cell + (y - rect.y) / Cell) % 2 == 0;
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(wxBrush(light ? wxColour(238, 238, 238) : wxColour(210, 210, 210)));
				dc.DrawRectangle(x, y, std::min(Cell, rect.GetRight() - x), std::min(Cell, rect.GetBottom() - y));
			}
		}

		dc.SetPen(wxPen(wxColour(92, 92, 92)));
		dc.SetBrush(wxBrush(color_));
		dc.DrawRectangle(rect);
	}

	wxColour color_ = wxColour(255, 255, 255, 255);
};

}

ColorAlphaDialog::ColorAlphaDialog(wxWindow* parent, const wxColour& initial_color) :
	wxDialog(parent, wxID_ANY, "Pick Color", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	color_(initial_color.IsOk() ? initial_color : wxColour(255, 255, 255, 255)) {
	auto* root = new wxBoxSizer(wxVERTICAL);

	auto* color_label = new wxStaticText(this, wxID_ANY, "Color");
	root->Add(color_label, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

	swatch_panel_ = new ColorSwatchPanel(this);
	swatch_panel_->SetCursor(wxCursor(wxCURSOR_HAND));
	root->Add(swatch_panel_, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

	auto* alpha_row = new wxBoxSizer(wxHORIZONTAL);
	alpha_row->Add(new wxStaticText(this, wxID_ANY, "Alpha"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

	alpha_slider_ = new wxSlider(this, wxID_ANY, color_.Alpha(), 0, 255, wxDefaultPosition, FromDIP(wxSize(220, -1)));
	alpha_row->Add(alpha_slider_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

	alpha_value_label_ = new wxStaticText(this, wxID_ANY, {});
	alpha_row->Add(alpha_value_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

	reset_alpha_button_ = new wxBitmapButton(this, wxID_ANY, wxNullBitmap, wxDefaultPosition, FromDIP(wxSize(30, 30)), wxBORDER_NONE);
	reset_alpha_button_->SetBitmap(IMAGE_MANAGER.GetBitmap(ICON_UNDO, wxSize(16, 16)));
	reset_alpha_button_->SetBitmapDisabled(IMAGE_MANAGER.GetBitmap(ICON_UNDO, wxSize(16, 16), wxColour(180, 180, 180)));
	alpha_row->Add(reset_alpha_button_, 0, wxALIGN_CENTER_VERTICAL);
	root->Add(alpha_row, 0, wxEXPAND | wxALL, FromDIP(10));

	root->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));

	SetSizerAndFit(root);
	SetMinSize(FromDIP(wxSize(320, 0)));

	UpdateSwatch();
	UpdateAlphaLabel();

	swatch_panel_->Bind(wxEVT_LEFT_UP, &ColorAlphaDialog::OnSwatchClicked, this);
	alpha_slider_->Bind(wxEVT_SLIDER, &ColorAlphaDialog::OnAlphaChanged, this);
	reset_alpha_button_->Bind(wxEVT_BUTTON, &ColorAlphaDialog::OnResetAlpha, this);
}

wxColour ColorAlphaDialog::GetColour() const {
	return color_;
}

void ColorAlphaDialog::OpenColorPicker() {
	wxColourData data;
	data.SetChooseFull(true);
	data.SetColour(wxColour(color_.Red(), color_.Green(), color_.Blue(), 255));

	wxColourDialog dialog(this, &data);
	if (dialog.ShowModal() != wxID_OK) {
		return;
	}

	const wxColour picked = dialog.GetColourData().GetColour();
	color_ = wxColour(picked.Red(), picked.Green(), picked.Blue(), color_.Alpha());
	UpdateSwatch();
}

void ColorAlphaDialog::UpdateSwatch() {
	if (auto* swatch = dynamic_cast<ColorSwatchPanel*>(swatch_panel_)) {
		swatch->SetColour(color_);
	}
}

void ColorAlphaDialog::UpdateAlphaLabel() {
	if (alpha_value_label_) {
		alpha_value_label_->SetLabel(wxString::Format("%d", static_cast<int>(color_.Alpha())));
	}
}

void ColorAlphaDialog::OnAlphaChanged(wxCommandEvent& event) {
	color_ = wxColour(color_.Red(), color_.Green(), color_.Blue(), static_cast<unsigned char>(alpha_slider_->GetValue()));
	UpdateSwatch();
	UpdateAlphaLabel();
	event.Skip();
}

void ColorAlphaDialog::OnResetAlpha(wxCommandEvent& event) {
	alpha_slider_->SetValue(DefaultAlpha);
	color_ = wxColour(color_.Red(), color_.Green(), color_.Blue(), static_cast<unsigned char>(DefaultAlpha));
	UpdateSwatch();
	UpdateAlphaLabel();
	event.Skip();
}

void ColorAlphaDialog::OnSwatchClicked(wxMouseEvent& event) {
	OpenColorPicker();
	event.Skip();
}
