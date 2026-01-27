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

#ifndef RME_UI_GAMES_SNAKE_PANEL_H_
#define RME_UI_GAMES_SNAKE_PANEL_H_

#include "ui/games/game_panel.h"

const int SNAKE_MAPHEIGHT = 20;
const int SNAKE_MAPWIDTH = 20;

class SnakePanel : public GamePanel {
public:
	SnakePanel(wxWindow* parent);
	~SnakePanel();

protected:
	virtual void Render(wxDC& pdc);
	virtual void GameLoop(int time);
	virtual void OnKey(wxKeyEvent& event, bool down);

	virtual int getFPS() const {
		return 7;
	}

	enum {
		NORTH,
		SOUTH,
		WEST,
		EAST,
	};

	void NewApple();
	void Move(int dir);
	void NewGame();
	void EndGame();
	void UpdateTitle();

	// -1 is apple, 0 is nothing, >0 is snake (will decay in n rounds)
	int length;
	int last_dir;
	int map[SNAKE_MAPWIDTH][SNAKE_MAPHEIGHT];
};

#endif
