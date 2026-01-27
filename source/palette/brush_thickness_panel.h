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

#ifndef RME_PALETTE_BRUSH_THICKNESS_PANEL_H_
#define RME_PALETTE_BRUSH_THICKNESS_PANEL_H_

#include "palette/palette_panel.h"
#include <wx/slider.h>

class BrushThicknessPanel : public PalettePanel {
public:
	BrushThicknessPanel(wxWindow* parent);
	~BrushThicknessPanel();

	// Interface
	wxString GetName() const;

	// Called when this page is displayed
	void OnSwitchIn();

	// wxWidgets event handling
	void OnScroll(wxScrollEvent& event);
	void OnClickCustomThickness(wxCommandEvent& event);

public:
	wxSlider* slider;
	wxCheckBox* use_button;

	DECLARE_EVENT_TABLE()
};

#endif
