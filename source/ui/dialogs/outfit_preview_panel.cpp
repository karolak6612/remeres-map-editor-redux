#include "ui/dialogs/outfit_preview_panel.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

OutfitPreviewPanel::OutfitPreviewPanel(wxWindow* parent, const Outfit& outfit) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, parent->FromDIP(wxSize(192, 192)), wxBORDER_NONE),
	preview_outfit(outfit),
	preview_direction(0) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_PAINT, &OutfitPreviewPanel::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &OutfitPreviewPanel::OnMouse, this);
	Bind(wxEVT_MOUSEWHEEL, &OutfitPreviewPanel::OnWheel, this);
	SetCursor(wxCursor(wxCURSOR_HAND));
	SetToolTip("Click or scroll to rotate character");
}

void OutfitPreviewPanel::SetOutfit(const Outfit& outfit) {
	preview_outfit = outfit;
	Refresh();
}

void OutfitPreviewPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(wxColour(230, 230, 230)));
	dc.Clear();

	GameSprite* spr = g_gui.gfx.getCreatureSprite(preview_outfit.lookType);
	if (spr) {
		Outfit draw_outfit = preview_outfit;
		// Map our internal 0-3 to Direction enum
		Direction dir = SOUTH;
		if (preview_direction == 1) {
			dir = EAST;
		} else if (preview_direction == 2) {
			dir = NORTH;
		} else if (preview_direction == 3) {
			dir = WEST;
		}

		wxBitmap bmp = SpriteIconGenerator::Generate(spr, SPRITE_SIZE_32x32, draw_outfit, false, dir);
		if (bmp.IsOk()) {
			std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
			if (gc) {
				gc->SetInterpolationQuality(wxINTERPOLATION_NONE);

				int sw = bmp.GetWidth();
				int sh = bmp.GetHeight();

				wxSize clientSize = GetClientSize();
				int w = clientSize.GetWidth();
				int h = clientSize.GetHeight();

				double scale = std::min<double>((double)w / sw, (double)h / sh);
				// Hero scale - make it fit nicely
				scale *= 0.9;

				double drawW = sw * scale;
				double drawH = sh * scale;
				double x = (w - drawW) / 2.0;
				double y = (h - drawH) / 2.0;

				gc->DrawBitmap(bmp, x, y, drawW, drawH);
			} else {
				dc.DrawBitmap(bmp, 0, 0);
			}
		}
	}
}

void OutfitPreviewPanel::OnMouse(wxMouseEvent& event) {
	preview_direction = (preview_direction + 1) % 4;
	Refresh();
}

void OutfitPreviewPanel::OnWheel(wxMouseEvent& event) {
	if (event.GetWheelRotation() > 0) {
		preview_direction = (preview_direction + 1) % 4;
	} else {
		preview_direction = (preview_direction + 3) % 4;
	}
	Refresh();
}
