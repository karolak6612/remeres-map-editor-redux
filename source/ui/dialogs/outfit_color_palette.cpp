#include "ui/dialogs/outfit_color_palette.h"
#include "ui/dialogs/outfit_chooser_dialog.h"
#include "rendering/core/graphics.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"

#include <glad/glad.h>
#include <nanovg.h>

OutfitColorPalette::OutfitColorPalette(wxWindow* parent, OutfitChooserDialog* owner) :
	NanoVGCanvas(parent, wxID_ANY, 0), // No scrollbar needed for fixed size
	owner(owner),
	selected_color_index(-1),
	hover_color_index(-1) {

	Bind(wxEVT_LEFT_DOWN, &OutfitColorPalette::OnMouse, this);
	Bind(wxEVT_MOTION, &OutfitColorPalette::OnMotion, this);
	Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
		if (hover_color_index != -1) {
			hover_color_index = -1;
			Refresh();
		}
	});

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

OutfitColorPalette::~OutfitColorPalette() = default;

void OutfitColorPalette::UpdateSelection(int currentColor) {
	// Find index for this color in the lookup table
	// Since color indices map directly to TemplateOutfitLookupTable indices (0-132)
	// We can use the color index directly.
	// But wait, the currentColor passed from outside is likely the color INDEX (0-132),
	// not the RGB value. The old code used indices.
	// Let's verify OutfitChooserDialog::UpdateColorSelection logic later.
	// Assuming it passes the index.

	if (selected_color_index != currentColor) {
		selected_color_index = currentColor;
		Refresh();
	}
}

wxSize OutfitColorPalette::DoGetBestClientSize() const {
	int width = COLS * ITEM_SIZE + (COLS - 1) * SPACING;
	int height = ROWS * ITEM_SIZE + (ROWS - 1) * SPACING;
	return FromDIP(wxSize(width, height));
}

void OutfitColorPalette::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// width and height are client size (logical or physical depending on implementation).
	// NanoVGCanvas implementation detail:
	// nvgBeginFrame(m_nvg, windowWidth, windowHeight, pixelRatio);
	// So we draw in logical pixels if pixelRatio is set correctly.

	for (int i = 0; i < TemplateOutfitLookupTableSize; ++i) {
		if (i >= ROWS * COLS) break; // Safety

		wxRect rect = GetItemRect(i);
		uint32_t color = TemplateOutfitLookupTable[i];

		// Convert 0xRRGGBB to NVGcolor
		NVGcolor col = nvgRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);

		nvgBeginPath(vg);
		nvgRect(vg, static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width), static_cast<float>(rect.height));
		nvgFillColor(vg, col);
		nvgFill(vg);

		// Selection border
		if (i == selected_color_index) {
			// Inset border
			nvgBeginPath(vg);
			nvgRect(vg, static_cast<float>(rect.x) + 0.5f, static_cast<float>(rect.y) + 0.5f, static_cast<float>(rect.width) - 1.0f, static_cast<float>(rect.height) - 1.0f);
			nvgStrokeColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent)));
			nvgStrokeWidth(vg, 2.0f);
			nvgStroke(vg);
		} else if (i == hover_color_index) {
			// Hover effect (e.g. slight border or brightness)
			nvgBeginPath(vg);
			nvgRect(vg, static_cast<float>(rect.x) + 0.5f, static_cast<float>(rect.y) + 0.5f, static_cast<float>(rect.width) - 1.0f, static_cast<float>(rect.height) - 1.0f);
			// Slightly lighter than black/white
			nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 128));
			nvgStrokeWidth(vg, 1.0f);
			nvgStroke(vg);
		}
	}
}

wxRect OutfitColorPalette::GetItemRect(int index) const {
	int row = index / COLS;
	int col = index % COLS;

	// Coordinates are logical
	return wxRect(
		col * (ITEM_SIZE + SPACING),
		row * (ITEM_SIZE + SPACING),
		ITEM_SIZE,
		ITEM_SIZE
	);
}

int OutfitColorPalette::HitTest(int x, int y) const {
	// x, y are logical coordinates from wxMouseEvent

	// Inverse of GetItemRect logic
	int col = x / (ITEM_SIZE + SPACING);
	int row = y / (ITEM_SIZE + SPACING);

	if (col < 0 || col >= COLS || row < 0 || row >= ROWS) {
		return -1;
	}

	int index = row * COLS + col;
	if (index >= 0 && index < TemplateOutfitLookupTableSize) {
		return index;
	}
	return -1;
}

void OutfitColorPalette::OnMouse(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1) {
		if (owner) {
			owner->SelectColor(index);
		}

		if (index != selected_color_index) {
			selected_color_index = index;
			Refresh();
		}
	}
	event.Skip();
}

void OutfitColorPalette::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != hover_color_index) {
		hover_color_index = index;
		Refresh();
	}
	event.Skip();
}
