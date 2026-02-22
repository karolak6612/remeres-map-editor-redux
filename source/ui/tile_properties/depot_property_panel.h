//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_DEPOT_PROPERTY_PANEL_H_
#define RME_UI_DEPOT_PROPERTY_PANEL_H_

#include "ui/tile_properties/item_property_panel.h"
#include <wx/choice.h>

class DepotPropertyPanel : public ItemPropertyPanel {
public:
	DepotPropertyPanel(wxWindow* parent);
	virtual ~DepotPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;

	void OnDepotIdChange(wxCommandEvent& event);

protected:
	wxStaticBoxSizer* depot_id_sizer;
	wxChoice* depot_id_field;
};

#endif
