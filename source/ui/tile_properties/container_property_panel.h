//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_UI_CONTAINER_PROPERTY_PANEL_H_
#define RME_UI_CONTAINER_PROPERTY_PANEL_H_

#include "ui/tile_properties/item_property_panel.h"
#include "ui/tile_properties/container_grid_canvas.h"

class ContainerPropertyPanel : public ItemPropertyPanel {
public:
	ContainerPropertyPanel(wxWindow* parent);
	virtual ~ContainerPropertyPanel();

	void SetItem(Item* item, Tile* tile, Map* map) override;

	void OnAddItem(wxCommandEvent& event);
	void OnEditItem(wxCommandEvent& event);
	void OnRemoveItem(wxCommandEvent& event);

	// Needed for ContainerGridCanvas callback
	void OnContainerItemClick(wxCommandEvent& event);
	void OnContainerItemRightClick(wxContextMenuEvent& event);

protected:
	void RebuildGrid();

	ContainerGridCanvas* grid_canvas;
};

#endif
