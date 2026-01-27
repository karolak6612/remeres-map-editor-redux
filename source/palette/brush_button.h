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

#ifndef RME_PALETTE_BRUSH_BUTTON_H_
#define RME_PALETTE_BRUSH_BUTTON_H_

#include "ui/controls/item_buttons.h"
#include "brushes/brush.h"

class BrushButton : public ItemToggleButton {
public:
	BrushButton(wxWindow* parent, Brush* brush, RenderSize, uint32_t id = wxID_ANY);
	virtual ~BrushButton();

	Brush* brush;

	void OnKey(wxKeyEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif
