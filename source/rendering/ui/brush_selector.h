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

#ifndef RME_BRUSH_SELECTOR_H_
#define RME_BRUSH_SELECTOR_H_

class Editor;
class Selection;
class Tile;
class GUI;
class Settings;

/**
 * @brief Handles brush selection from popup menu context.
 *
 * Extracts all OnSelect*Brush() methods from MapCanvas to improve
 * separation of concerns.
 */
class BrushSelector {
public:
	static void SelectRAWBrush(GUI& gui, Selection& selection);
	static void SelectGroundBrush(GUI& gui, Selection& selection);
	static void SelectDoodadBrush(GUI& gui, Selection& selection);
	static void SelectDoorBrush(GUI& gui, Selection& selection);
	static void SelectWallBrush(GUI& gui, Selection& selection);
	static void SelectCarpetBrush(GUI& gui, Selection& selection);
	static void SelectTableBrush(GUI& gui, Selection& selection);
	static void SelectHouseBrush(GUI& gui, Editor& editor, Selection& selection);
	static void SelectCollectionBrush(GUI& gui, Selection& selection);
	static void SelectCreatureBrush(GUI& gui, Selection& selection);
	static void SelectSpawnBrush(GUI& gui);
	static void SelectSmartBrush(GUI& gui, const Settings& settings, Editor& editor, Tile* tile);

private:
	BrushSelector() = delete;
};

#endif // RME_BRUSH_SELECTOR_H_
