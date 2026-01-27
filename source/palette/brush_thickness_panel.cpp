//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "palette/brush_thickness_panel.h"
#include "palette/managers/palette_manager.h"
#include "brushes/managers/brush_manager.h"
#include "ui/gui_ids.h"

// ============================================================================
// Brush Thickness Panel

BEGIN_EVENT_TABLE(BrushThicknessPanel, PalettePanel)
#ifdef __WINDOWS__
// This only works in wxmsw
EVT_COMMAND_SCROLL_CHANGED(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
#else
EVT_COMMAND_SCROLL_TOP(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_BOTTOM(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_LINEUP(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_LINEDOWN(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_PAGEUP(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_PAGEDOWN(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
EVT_COMMAND_SCROLL_THUMBRELEASE(PALETTE_DOODAD_SLIDER, BrushThicknessPanel::OnScroll)
#endif

EVT_CHECKBOX(PALETTE_DOODAD_USE_THICKNESS, BrushThicknessPanel::OnClickCustomThickness)
END_EVENT_TABLE()

BrushThicknessPanel::BrushThicknessPanel(wxWindow* parent) :
	PalettePanel(parent, wxID_ANY) {
	wxSizer* thickness_sizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* thickness_sub_sizer = newd wxBoxSizer(wxHORIZONTAL);
	thickness_sub_sizer->Add(20, 10);
	use_button = newd wxCheckBox(this, PALETTE_DOODAD_USE_THICKNESS, "Use custom thickness");
	thickness_sub_sizer->Add(use_button);
	thickness_sizer->Add(thickness_sub_sizer, 1, wxEXPAND);

	slider = newd wxSlider(this, PALETTE_DOODAD_SLIDER, 5, 1, 10, wxDefaultPosition);
	thickness_sizer->Add(slider, 1, wxEXPAND);

	SetSizerAndFit(thickness_sizer);
}

BrushThicknessPanel::~BrushThicknessPanel() {
	////
}

wxString BrushThicknessPanel::GetName() const {
	return "Brush Thickness";
}

void BrushThicknessPanel::OnScroll(wxScrollEvent& event) {
	static const int lookup_table[10] = { 1, 2, 3, 5, 8, 13, 23, 35, 50, 80 };
	use_button->SetValue(true);

	ASSERT(event.GetPosition() >= 1);
	ASSERT(event.GetPosition() <= 10);

	g_palettes.ActivatePalette(GetParentPalette());
	g_brush_manager.SetBrushThickness(true, lookup_table[event.GetPosition() - 1], 100);
}

void BrushThicknessPanel::OnClickCustomThickness(wxCommandEvent& event) {
	g_palettes.ActivatePalette(GetParentPalette());
	g_brush_manager.SetBrushThickness(event.IsChecked());
}

void BrushThicknessPanel::OnSwitchIn() {
	static const int lookup_table[10] = { 1, 2, 3, 5, 8, 13, 23, 35, 50, 80 };
	g_palettes.ActivatePalette(GetParentPalette());
	g_brush_manager.SetBrushThickness(lookup_table[slider->GetValue() - 1], 100);
}
