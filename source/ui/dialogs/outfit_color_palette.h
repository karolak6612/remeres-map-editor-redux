#ifndef RME_UI_DIALOGS_OUTFIT_COLOR_PALETTE_H_
#define RME_UI_DIALOGS_OUTFIT_COLOR_PALETTE_H_

#include "app/main.h"
#include "util/nanovg_canvas.h"
#include "rendering/core/outfit_colors.h"

class OutfitChooserDialog;

class OutfitColorPalette : public NanoVGCanvas {
public:
	OutfitColorPalette(wxWindow* parent, OutfitChooserDialog* owner);
	virtual ~OutfitColorPalette();

	void UpdateSelection(int currentColor);

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

	void OnMouse(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);

private:
	OutfitChooserDialog* owner;
	int selected_color_index;
	int hover_color_index;

	// Layout constants
	static const int ROWS = 7;
	static const int COLS = 19;
	static const int ITEM_SIZE = 16;
	static const int SPACING = 1;

	int HitTest(int x, int y) const;
	wxRect GetItemRect(int index) const;
};

#endif // RME_UI_DIALOGS_OUTFIT_COLOR_PALETTE_H_
