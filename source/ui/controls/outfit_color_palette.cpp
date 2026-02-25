#include "ui/controls/outfit_color_palette.h"
#include "rendering/core/outfit_colors.h"
#include "ui/core/theme.h"
#include "util/nvg_utils.h"

#include <nanovg.h>
#include <algorithm>

OutfitColorPalette::OutfitColorPalette(wxWindow* parent, wxWindowID id) :
	NanoVGCanvas(parent, id, wxBORDER_NONE) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	Bind(wxEVT_LEFT_DOWN, &OutfitColorPalette::OnMouse, this);
	Bind(wxEVT_MOTION, &OutfitColorPalette::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, &OutfitColorPalette::OnLeave, this);
}

void OutfitColorPalette::SetSelectedColor(int index) {
	if (index >= 0 && index < static_cast<int>(TemplateOutfitLookupTableSize)) {
		m_selectedIndex = index;
		Refresh();
	}
}

void OutfitColorPalette::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	int cellSize = FROM_DIP(this, CELL_SIZE);
	int padding = FROM_DIP(this, PADDING);

	for (int i = 0; i < static_cast<int>(TemplateOutfitLookupTableSize); ++i) {
		int row = i / COLS;
		int col = i % COLS;

		float x = static_cast<float>(col * (cellSize + padding) + padding);
		float y = static_cast<float>(row * (cellSize + padding) + padding);

		uint32_t color = TemplateOutfitLookupTable[i];

		// Inner rect
		nvgBeginPath(vg);
		nvgRect(vg, x, y, static_cast<float>(cellSize), static_cast<float>(cellSize));
		nvgFillColor(vg, nvgRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF));
		nvgFill(vg);

		// Highlight
		if (i == m_selectedIndex) {
			float strokeWidth = std::max(1.0f, static_cast<float>(FROM_DIP(this, 2)));
			float offset = strokeWidth / 2.0f + 0.5f; // Push out slightly

			nvgBeginPath(vg);
			nvgRect(vg, x - offset, y - offset, cellSize + offset * 2, cellSize + offset * 2);
			nvgStrokeColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
			nvgStrokeWidth(vg, strokeWidth);
			nvgStroke(vg);
		} else if (i == m_hoverIndex) {
			float strokeWidth = std::max(1.0f, static_cast<float>(FROM_DIP(this, 1)));
			float offset = strokeWidth / 2.0f;

			nvgBeginPath(vg);
			nvgRect(vg, x - offset, y - offset, cellSize + offset * 2, cellSize + offset * 2);
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 128));
			nvgStrokeWidth(vg, strokeWidth);
			nvgStroke(vg);
		}
	}
}

wxSize OutfitColorPalette::DoGetBestClientSize() const {
	int cellSize = FROM_DIP(this, CELL_SIZE);
	int padding = FROM_DIP(this, PADDING);
	return wxSize(COLS * (cellSize + padding) + padding, ROWS * (cellSize + padding) + padding);
}

int OutfitColorPalette::HitTest(int x, int y) const {
	int cellSize = FROM_DIP(this, CELL_SIZE);
	int padding = FROM_DIP(this, PADDING);

	if (x < padding || y < padding) {
		return -1;
	}

	int offsetX = (x - padding) % (cellSize + padding);
	int offsetY = (y - padding) % (cellSize + padding);

	if (offsetX >= cellSize || offsetY >= cellSize) {
		return -1;
	}

	int col = (x - padding) / (cellSize + padding);
	int row = (y - padding) / (cellSize + padding);

	if (col >= 0 && col < COLS && row >= 0 && row < ROWS) {
		int index = row * COLS + col;
		if (index < static_cast<int>(TemplateOutfitLookupTableSize)) {
			return index;
		}
	}
	return -1;
}

void OutfitColorPalette::OnMouse(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1) {
		m_selectedIndex = index;
		Refresh();

		wxCommandEvent evt(wxEVT_BUTTON, GetId());
		evt.SetEventObject(this);
		evt.SetInt(index); // Pass the selected index
		ProcessEvent(evt);
	}
}

void OutfitColorPalette::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != m_hoverIndex) {
		m_hoverIndex = index;
		Refresh();
	}
	event.Skip();
}

void OutfitColorPalette::OnLeave(wxMouseEvent& event) {
	if (m_hoverIndex != -1) {
		m_hoverIndex = -1;
		Refresh();
	}
	event.Skip();
}
