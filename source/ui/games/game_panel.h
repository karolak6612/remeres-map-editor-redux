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

#ifndef RME_UI_GAMES_GAME_PANEL_H_
#define RME_UI_GAMES_GAME_PANEL_H_

#include "app/main.h"

class GamePanel : public wxPanel {
public:
	GamePanel(wxWindow* parent, int width, int height);
	virtual ~GamePanel();

	void OnPaint(wxPaintEvent&);
	void OnKeyDown(wxKeyEvent&);
	void OnKeyUp(wxKeyEvent&);
	void OnIdle(wxIdleEvent&);

	void pause() {
		paused_val = true;
	}
	void unpause() {
		paused_val = false;
	}
	bool paused() const {
		return paused_val || dead;
	}

protected:
	virtual void Render(wxDC& pdc) = 0;
	virtual void GameLoop(int time) = 0;
	virtual void OnKey(wxKeyEvent& event, bool down) = 0;

	virtual int getFPS() const = 0;

protected:
	wxStopWatch game_timer;

private:
	bool paused_val;

	DECLARE_EVENT_TABLE()

protected:
	bool dead;
};

#endif
