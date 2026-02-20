//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_SPAWN_CREATURE_PANEL_H_
#define RME_UI_SPAWN_CREATURE_PANEL_H_

#include <wx/panel.h>
#include <functional>

class Tile;
class Map;

class SpawnCreaturePanel : public wxPanel {
public:
	SpawnCreaturePanel(wxWindow* parent);
	virtual ~SpawnCreaturePanel();

	void SetTile(Tile* tile, Map* map);

	void SetOnSpawnSelectedCallback(std::function<void()> cb) {
		on_spawn_selected_cb = cb;
	}
	void SetOnCreatureSelectedCallback(std::function<void()> cb) {
		on_creature_selected_cb = cb;
	}

protected:
	void OnSpawnClick(wxMouseEvent& event);
	void OnCreatureClick(wxMouseEvent& event);

	std::function<void()> on_spawn_selected_cb;
	std::function<void()> on_creature_selected_cb;

	wxStaticBitmap* spawn_bitmap;
	wxStaticBitmap* creature_bitmap;
	wxStaticText* spawn_text;
	wxStaticText* creature_text;
};

#endif
