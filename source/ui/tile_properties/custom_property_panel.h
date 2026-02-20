//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_CUSTOM_PROPERTY_PANEL_H_
#define RME_UI_CUSTOM_PROPERTY_PANEL_H_

#include <wx/wx.h>
#include <wx/panel.h>

class Tile;
class Map;
class Item;

class CustomPropertyPanel : public wxPanel {
public:
	CustomPropertyPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) { }
	virtual ~CustomPropertyPanel() { }

	virtual void SetItem(Item* item, Tile* tile, Map* map) = 0;
};

#endif
