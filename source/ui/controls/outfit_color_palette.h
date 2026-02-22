#ifndef RME_UI_CONTROLS_OUTFIT_COLOR_PALETTE_H_
#define RME_UI_CONTROLS_OUTFIT_COLOR_PALETTE_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include <wx/event.h>

class OutfitColorPalette : public NanoVGCanvas {
public:
	OutfitColorPalette(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~OutfitColorPalette();

	void SetSelectedColor(int index);
	int GetSelectedColor() const {
		return m_selectedIndex;
	}

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

private:
	void OnMouse(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);

	int HitTest(int x, int y) const;

	int m_selectedIndex = 0;
	int m_hoverIndex = -1;

	// Grid layout constants (logical pixels, handled by NanoVG scaling)
	static const int ROWS = 7;
	static const int COLS = 19;
	static const int CELL_SIZE = 14;
	static const int PADDING = 2;
};

#endif
