#ifndef RME_UI_CONTROLS_OUTFIT_COLOR_PALETTE_H_
#define RME_UI_CONTROLS_OUTFIT_COLOR_PALETTE_H_

#include "app/main.h"
#include "rendering/core/light_defaults.h"
#include "util/nanovg_canvas.h"
#include <wx/event.h>

class OutfitColorPalette : public NanoVGCanvas {
public:
	OutfitColorPalette(wxWindow* parent, wxWindowID id = wxID_ANY);
	~OutfitColorPalette() override = default;

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
	static constexpr int ROWS = 7;
	static constexpr int COLS = 19;
	static constexpr int CELL_SIZE = 14;
	static constexpr int PADDING = 2;
};

class LightColorPalette : public NanoVGCanvas {
public:
	LightColorPalette(wxWindow* parent, wxWindowID id = wxID_ANY);
	~LightColorPalette() override = default;

	void SetSelectedColor(int index);
	[[nodiscard]] int GetSelectedColor() const {
		return selected_index_;
	}

protected:
	void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
	wxSize DoGetBestClientSize() const override;

private:
	void OnMouse(wxMouseEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	[[nodiscard]] int HitTest(int x, int y) const;

	int selected_index_ = rme::lighting::DEFAULT_SERVER_LIGHT_COLOR;
	int hover_index_ = -1;

	static constexpr int ROWS = 12;
	static constexpr int COLS = 18;
	static constexpr int CELL_SIZE = 14;
	static constexpr int PADDING = 2;
};

class wxStaticBitmap;
class wxStaticText;

class LightColorPickerDialog : public wxDialog {
public:
	LightColorPickerDialog(wxWindow* parent, int current_color);
	~LightColorPickerDialog() override = default;

	[[nodiscard]] int GetSelectedColor() const {
		return selected_color_;
	}

private:
	void UpdateSelection(int color_index);
	void OnPaletteSelection(wxCommandEvent& event);

	wxStaticBitmap* preview_bitmap_ = nullptr;
	wxStaticText* selection_label_ = nullptr;
	LightColorPalette* palette_ = nullptr;
	int selected_color_ = 0;
};

[[nodiscard]] wxBitmap CreateLightColorSwatchBitmap(const wxWindow* window, int color_index, const wxSize& size);

#endif
