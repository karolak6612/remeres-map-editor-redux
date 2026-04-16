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

#ifndef RME_PALETTE_CREATURE_H_
#define RME_PALETTE_CREATURE_H_

#include "brushes/brush_enums.h"
#include "palette/palette_common.h"
#include "palette/panels/brush_panel.h"
#include "ui/controls/sortable_list_box.h"

class wxSearchCtrl;

class CreaturePalettePanel : public PalettePanel {
public:
	CreaturePalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	~CreaturePalettePanel() override = default;

	PaletteType GetType() const override;
	[[nodiscard]] Brush* GetSelectedCreatureBrush() const;

	void SelectFirstBrush() override;
	Brush* GetSelectedBrush() const override;
	int GetSelectedBrushSize() const override;
	bool SelectBrush(const Brush* whatbrush) override;

	void OnUpdateBrushSize(BrushShape shape, int size) override;
	void OnSwitchIn() override;
	void OnUpdate() override;
	void OnRefreshTilesets();

	void SetListType(BrushListType ltype);
	void SetListType(wxString ltype);
	[[nodiscard]] bool IsNpcPageSelected() const;

protected:
	void SelectTileset(size_t index);
	void SelectCreature(size_t index);
	void SelectCreature(std::string name);
	void SyncSpawnControlsToSelection() const;
	void SelectCreatureFromSearch(const wxString& query);

public:
	void OnSwitchingPage(wxChoicebookEvent& event);
	void OnPageChanged(wxChoicebookEvent& event);
	void OnSearchChanged(wxCommandEvent& event);

	wxSearchCtrl* search_ctrl = nullptr;
	wxChoicebook* choicebook = nullptr;
};

#endif
