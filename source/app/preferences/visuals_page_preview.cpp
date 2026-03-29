#include "app/preferences/visuals_page_preview.h"

#include "item_definitions/core/item_definition_store.h"
#include "rendering/core/game_sprite.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "ui/gui.h"
#include "util/image_manager.h"

#include <algorithm>

#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

namespace {

uint16_t ResolveClientIdFromItem(uint16_t item_id) {
	if (item_id == 0) {
		return 0;
	}
	if (const auto definition = g_item_definitions.get(item_id)) {
		return definition.clientId();
	}
	return 0;
}

wxBitmap ApplyPreviewTint(wxBitmap bitmap, const wxColour& tint) {
	if (!bitmap.IsOk() || !tint.IsOk()) {
		return bitmap;
	}

	const bool neutral_rgb = tint.Red() == 255 && tint.Green() == 255 && tint.Blue() == 255;
	const bool neutral_alpha = tint.Alpha() == 255;
	if (neutral_rgb && neutral_alpha) {
		return bitmap;
	}

	wxImage image = bitmap.ConvertToImage();
	if (!image.IsOk()) {
		return bitmap;
	}

	if (!image.HasAlpha()) {
		image.InitAlpha();
	}

	unsigned char* data = image.GetData();
	unsigned char* alpha = image.GetAlpha();
	if (!data || !alpha) {
		return bitmap;
	}

	const int pixel_count = image.GetWidth() * image.GetHeight();
	for (int index = 0; index < pixel_count; ++index) {
		data[index * 3 + 0] = static_cast<unsigned char>((data[index * 3 + 0] * tint.Red()) / 255);
		data[index * 3 + 1] = static_cast<unsigned char>((data[index * 3 + 1] * tint.Green()) / 255);
		data[index * 3 + 2] = static_cast<unsigned char>((data[index * 3 + 2] * tint.Blue()) / 255);
		alpha[index] = static_cast<unsigned char>((alpha[index] * tint.Alpha()) / 255);
	}

	return wxBitmap(image);
}

wxBitmap BuildAppearanceBitmap(const VisualAppearance& appearance, const wxSize& size = wxSize(48, 48)) {
	switch (appearance.type) {
		case VisualAppearanceType::Rgba: {
			wxBitmap bitmap(size.GetWidth(), size.GetHeight());
			wxMemoryDC dc(bitmap);
			dc.SetBackground(wxBrush(wxColour(24, 24, 24)));
			dc.Clear();
			dc.SetPen(wxPen(wxColour(70, 70, 70)));
			const int inset_x = std::max(4, size.GetWidth() / 6);
			const int inset_y = std::max(4, size.GetHeight() / 6);
			dc.SetBrush(wxBrush(appearance.color));
			dc.DrawRectangle(inset_x, inset_y, std::max(8, size.GetWidth() - inset_x * 2), std::max(8, size.GetHeight() - inset_y * 2));
			dc.SelectObject(wxNullBitmap);
			return bitmap;
		}
		case VisualAppearanceType::SpriteId:
			if (appearance.sprite_id != 0) {
				if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(static_cast<uint32_t>(appearance.sprite_id)))) {
					return ApplyPreviewTint(SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false), appearance.color);
				}
			}
			break;
		case VisualAppearanceType::OtherItemVisual:
			if (const uint16_t client_id = ResolveClientIdFromItem(appearance.item_id); client_id != 0) {
				if (auto* sprite = dynamic_cast<GameSprite*>(g_gui.gfx.getSprite(client_id))) {
					return ApplyPreviewTint(SpriteIconGenerator::Generate(sprite, SPRITE_SIZE_32x32, false), appearance.color);
				}
			}
			break;
		case VisualAppearanceType::Png:
		case VisualAppearanceType::Svg:
			if (!appearance.asset_path.empty()) {
				return ApplyPreviewTint(
					IMAGE_MANAGER.GetBitmap(appearance.asset_path, size, Visuals::EffectiveImageTint(appearance.color)),
					wxColour(255, 255, 255, appearance.color.Alpha())
				);
			}
			break;
	}

	return wxArtProvider::GetBitmap(wxART_MISSING_IMAGE, wxART_OTHER, size);
}

wxRect CenteredRect(const wxRect& bounds, const wxSize& size) {
	return wxRect(
		bounds.x + std::max(0, (bounds.width - size.GetWidth()) / 2),
		bounds.y + std::max(0, (bounds.height - size.GetHeight()) / 2),
		size.GetWidth(),
		size.GetHeight()
	);
}

void DrawCheckerboard(wxDC& dc, const wxRect& rect) {
	const int cell = 10;
	for (int y = rect.y; y < rect.GetBottom(); y += cell) {
		for (int x = rect.x; x < rect.GetRight(); x += cell) {
			const bool light = ((x - rect.x) / cell + (y - rect.y) / cell) % 2 == 0;
			dc.SetBrush(wxBrush(light ? wxColour(34, 34, 34) : wxColour(26, 26, 26)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(x, y, std::min(cell, rect.GetRight() - x), std::min(cell, rect.GetBottom() - y));
		}
	}
}

void DrawTileBase(wxDC& dc, const wxRect& rect) {
	DrawCheckerboard(dc, rect);

	const wxRect tile = CenteredRect(rect, wxSize(std::min(rect.width - 16, 88), std::min(rect.height - 16, 88)));
	dc.SetPen(wxPen(wxColour(95, 95, 95)));
	dc.SetBrush(wxBrush(wxColour(58, 62, 68)));
	dc.DrawRoundedRectangle(tile, 8);

	dc.SetPen(wxPen(wxColour(78, 82, 88)));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	wxRect inset_tile = tile;
	inset_tile.Deflate(4);
	dc.DrawRoundedRectangle(inset_tile, 6);
}

wxRect PreviewTileRect(const wxRect& rect) {
	return CenteredRect(rect, wxSize(std::min(rect.width - 16, 88), std::min(rect.height - 16, 88)));
}

wxPoint OverlayPreviewCenter(const VisualRule& rule, const wxRect& tile, const wxSize& size) {
	int x = tile.x + (tile.width - size.GetWidth()) / 2;
	int y = tile.y + (tile.height - size.GetHeight()) / 2;

	if (rule.match_type == VisualMatchType::Overlay) {
		if (rule.match_value == "door_locked" || rule.match_value == "door_unlocked") {
			y = tile.y + (tile.height - size.GetHeight()) / 4;
		} else if (rule.match_value == "hook_south") {
			y = tile.y + tile.height - size.GetHeight() - 8;
		} else if (rule.match_value == "hook_east") {
			x = tile.x + tile.width - size.GetWidth() - 8;
		}
	}

	return wxPoint(x, y);
}

void DrawAppearanceOnTile(wxDC& dc, const wxRect& tile, const VisualRule& rule) {
	const VisualAppearance& appearance = rule.appearance;
	if (appearance.type == VisualAppearanceType::Rgba) {
		wxRect target = tile;
		target.Deflate(rule.match_type == VisualMatchType::Tile ? 6 : 18);
		if (rule.match_type == VisualMatchType::Overlay) {
			if (rule.match_value == "hook_south") {
				target = wxRect(tile.x + tile.width / 2 - 14, tile.y + tile.height - 28, 28, 18);
			} else if (rule.match_value == "hook_east") {
				target = wxRect(tile.x + tile.width - 28, tile.y + tile.height / 2 - 14, 18, 28);
			} else if (rule.match_value == "door_locked" || rule.match_value == "door_unlocked") {
				target = wxRect(tile.x + tile.width / 2 - 14, tile.y + 10, 28, 28);
			} else {
				target = wxRect(tile.x + tile.width / 2 - 12, tile.y + tile.height / 2 - 12, 24, 24);
			}
		}

		dc.SetPen(wxPen(wxColour(0, 0, 0, std::min(255, appearance.color.Alpha() + 20))));
		dc.SetBrush(wxBrush(appearance.color));
		dc.DrawRoundedRectangle(target, 6);
		return;
	}

	const wxSize bitmap_size(rule.match_type == VisualMatchType::Tile ? 72 : 56, rule.match_type == VisualMatchType::Tile ? 72 : 56);
	const wxBitmap bitmap = BuildAppearanceBitmap(appearance, bitmap_size);
	if (!bitmap.IsOk()) {
		return;
	}

	wxPoint draw_pos = OverlayPreviewCenter(rule, tile, bitmap.GetSize());
	if (rule.match_type == VisualMatchType::Tile && rule.match_value == "house_overlay") {
		draw_pos = wxPoint(tile.x + (tile.width - bitmap.GetWidth()) / 2, tile.y + (tile.height - bitmap.GetHeight()) / 2);
	}

	dc.DrawBitmap(bitmap, draw_pos.x, draw_pos.y, true);
}

}

VisualsPreviewPanel::VisualsPreviewPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(56, 56)) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(56, 56));
	Bind(wxEVT_PAINT, &VisualsPreviewPanel::OnPaint, this);
}

void VisualsPreviewPanel::SetRule(std::optional<VisualRule> rule) {
	rule_ = std::move(rule);
	Refresh();
}

void VisualsPreviewPanel::OnPaint(wxPaintEvent&) {
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();

	wxRect bounds = GetClientRect();
	bounds.Deflate(4);
	dc.SetPen(wxPen(wxColour(88, 88, 88)));
	dc.SetBrush(wxBrush(wxColour(20, 20, 20)));
	dc.DrawRoundedRectangle(bounds, FromDIP(6));

	if (!rule_.has_value()) {
		return;
	}

	wxRect scene_rect = bounds;
	scene_rect.Deflate(6);
	DrawTileBase(dc, scene_rect);
	DrawAppearanceOnTile(dc, PreviewTileRect(scene_rect), *rule_);
}
