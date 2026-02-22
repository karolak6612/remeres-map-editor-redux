//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_CONTAINER_GRID_CANVAS_H_
#define RME_CONTAINER_GRID_CANVAS_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <vector>

class Item;

class ContainerGridCanvas : public NanoVGCanvas {
public:
	ContainerGridCanvas(wxWindow* parent, bool large);
	virtual ~ContainerGridCanvas();

	void SetContainer(Item* container);
	Item* GetContainer() const {
		return m_container;
	}

	// Gets the selected item index inside the container
	int GetSelectedIndex() const {
		return m_selected_index;
	}
	void SetSelectedIndex(int index);

	// Toggles large icons
	void SetLargeSprites(bool large);

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void DrawSunkenBorder(NVGcontext* vg, float x, float y, float size_x, float size_y);
	void DrawRaisedBorder(NVGcontext* vg, float x, float y, float size_x, float size_y);
	void DrawHoverEffects(NVGcontext* vg, float x, float y, float size_x, float size_y);

	void OnMotion(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnRightDown(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);

	int HitTest(int x, int y) const;
	void RecalculateLayout();

	Item* m_container;

	bool m_large;
	int m_hover_index;
	int m_selected_index;

	// Layout metrics
	int m_cols;
	int m_rows;
	float m_slot_size;
	float m_padding;
};

#endif
