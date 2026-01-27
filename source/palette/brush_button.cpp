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
#include "palette/brush_button.h"
#include "ui/gui.h"

// ============================================================================
// Brush Button

BEGIN_EVENT_TABLE(BrushButton, ItemToggleButton)
EVT_KEY_DOWN(BrushButton::OnKey)
END_EVENT_TABLE()

BrushButton::BrushButton(wxWindow* parent, Brush* _brush, RenderSize sz, uint32_t id) :
	ItemToggleButton(parent, sz, uint16_t(0), id),
	brush(_brush) {
	ASSERT(sz != RENDER_SIZE_64x64);
	ASSERT(brush);
	SetSprite(brush->getLookID());
	SetToolTip(wxstr(brush->getName()));
}

BrushButton::~BrushButton() {
	////
}

void BrushButton::OnKey(wxKeyEvent& event) {
	g_gui.AddPendingCanvasEvent(event);
}
