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
#include "ui/games/game_panel.h"
#include <wx/dcbuffer.h>

BEGIN_EVENT_TABLE(GamePanel, wxPanel)
EVT_KEY_DOWN(GamePanel::OnKeyDown)
EVT_KEY_UP(GamePanel::OnKeyUp)
EVT_PAINT(GamePanel::OnPaint)
EVT_IDLE(GamePanel::OnIdle)
END_EVENT_TABLE()

GamePanel::GamePanel(wxWindow* parent, int width, int height) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(width, height), wxWANTS_CHARS),
	paused_val(false),
	dead(false) {
	// Receive idle events
	SetExtraStyle(wxWS_EX_PROCESS_IDLE);
	// Complete redraw
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

GamePanel::~GamePanel() {
	////
}

void GamePanel::OnPaint(wxPaintEvent&) {
	wxBufferedPaintDC pdc(this);
	Render(pdc);
}

void GamePanel::OnKeyDown(wxKeyEvent& event) {
	switch (event.GetKeyCode()) {
		case WXK_ESCAPE: {
			if (dead) {
				return;
			}
			wxDialog* dlg = (wxDialog*)GetParent();
			dlg->EndModal(0);
			break;
		}
		default: {
			OnKey(event, true);
			break;
		}
	}
}

void GamePanel::OnKeyUp(wxKeyEvent& event) {
	OnKey(event, false);
}

void GamePanel::OnIdle(wxIdleEvent& event) {
	int time = game_timer.Time();
	if (time > 1000 / getFPS()) {
		game_timer.Start();
		if (!paused()) {
			GameLoop(time);
		}
	}
	if (!paused()) {
		event.RequestMore(true);
	}
}
