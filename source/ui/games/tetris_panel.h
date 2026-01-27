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

#ifndef RME_UI_GAMES_TETRIS_PANEL_H_
#define RME_UI_GAMES_TETRIS_PANEL_H_

#include "ui/games/game_panel.h"

const int TETRIS_MAPHEIGHT = 20;
const int TETRIS_MAPWIDTH = 10;

class TetrisPanel : public GamePanel {
public:
	TetrisPanel(wxWindow* parent);
	~TetrisPanel();

protected:
	virtual void Render(wxDC& pdc);
	virtual void GameLoop(int time);
	virtual void OnKey(wxKeyEvent& event, bool down);

	virtual int getFPS() const {
		return lines / 10 + 3;
	}

	enum Color {
		NO_COLOR,
		RED,
		BLUE,
		GREEN,
		STEEL,
		YELLOW,
		PURPLE,
		WHITE,
	};

	enum BlockType {
		FIRST_BLOCK,
		BLOCK_TOWER = FIRST_BLOCK,
		BLOCK_SQUARE,
		BLOCK_TRIANGLE,
		BLOCK_L,
		BLOCK_J,
		BLOCK_Z,
		BLOCK_S,
		LAST_BLOCK = BLOCK_S
	};

	struct Block {
		Color structure[4][4];
		int x, y;
	} block;

	const wxBrush& GetBrush(Color color) const;
	bool BlockCollisionTest(int mx, int my) const;
	void RemoveRow(int row);
	void NewBlock();
	void MoveBlock(int x, int y);
	void RotateBlock();
	void NewGame();
	void EndGame();
	void AddScore(int lines);

	int score;
	int lines;
	Color map[TETRIS_MAPWIDTH][TETRIS_MAPHEIGHT];
};

#endif
