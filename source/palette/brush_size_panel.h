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

#ifndef RME_PALETTE_BRUSH_SIZE_PANEL_H_
#define RME_PALETTE_BRUSH_SIZE_PANEL_H_

#include "palette/palette_panel.h"
#include "ui/dcbutton.h"

class BrushSizePanel : public PalettePanel {
public:
	BrushSizePanel(wxWindow* parent);
	~BrushSizePanel() { }

	// Interface
	// Flushes this panel and consequent views will feature reloaded data
	void InvalidateContents();
	// Loads the currently displayed page
	void LoadCurrentContents();
	// Loads all content in this panel
	void LoadAllContents();

	wxString GetName() const;
	void SetToolbarIconSize(bool large);

	// Updates the palette window to use the current brush size
	void OnUpdateBrushSize(BrushShape shape, int size);
	// Called when this page is displayed
	void OnSwitchIn();

	// wxWidgets event handling
	void OnClickSquareBrush(wxCommandEvent& event);
	void OnClickCircleBrush(wxCommandEvent& event);

	void OnClickBrushSize(int which);
	void OnClickBrushSize0(wxCommandEvent& event) {
		OnClickBrushSize(0);
	}
	void OnClickBrushSize1(wxCommandEvent& event) {
		OnClickBrushSize(1);
	}
	void OnClickBrushSize2(wxCommandEvent& event) {
		OnClickBrushSize(2);
	}
	void OnClickBrushSize4(wxCommandEvent& event) {
		OnClickBrushSize(4);
	}
	void OnClickBrushSize6(wxCommandEvent& event) {
		OnClickBrushSize(6);
	}
	void OnClickBrushSize8(wxCommandEvent& event) {
		OnClickBrushSize(8);
	}
	void OnClickBrushSize11(wxCommandEvent& event) {
		OnClickBrushSize(11);
	}

protected:
	bool loaded;
	bool large_icons;

	DCButton* brushshapeSquareButton;
	DCButton* brushshapeCircleButton;

	DCButton* brushsize0Button;
	DCButton* brushsize1Button;
	DCButton* brushsize2Button;
	DCButton* brushsize4Button;
	DCButton* brushsize6Button;
	DCButton* brushsize8Button;
	DCButton* brushsize11Button;

	DECLARE_EVENT_TABLE()
};

#endif
