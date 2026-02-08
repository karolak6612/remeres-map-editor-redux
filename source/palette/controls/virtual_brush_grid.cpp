#include "app/main.h"
#include "palette/controls/virtual_brush_grid.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"

#include <glad/glad.h>

#include <nanovg.h>
#include <nanovg_gl.h>

#include <spdlog/spdlog.h>

VirtualBrushGrid::VirtualBrushGrid(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz) :
	NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
	BrushBoxInterface(_tileset),
	icon_size(rsz),
	display_mode(rsz == RENDER_SIZE_16x16 ? BRUSHLIST_SMALL_ICONS : BRUSHLIST_LARGE_ICONS),
	selected_index(-1),
	hover_index(-1),
	columns(1),
	item_size(0),
	padding(4),
	m_animTimer(new wxTimer(this)) {

	Bind(wxEVT_LEFT_DOWN, &VirtualBrushGrid::OnMouseDown, this);
	Bind(wxEVT_MOTION, &VirtualBrushGrid::OnMotion, this);
	Bind(wxEVT_TIMER, &VirtualBrushGrid::OnTimer, this);

	UpdateLayout();
}

VirtualBrushGrid::~VirtualBrushGrid() {
	m_animTimer->Stop();
	delete m_animTimer;
}

void VirtualBrushGrid::SetDisplayMode(BrushListType mode) {
	if (display_mode != mode) {
		display_mode = mode;
		UpdateLayout();
		Refresh();
	}
}

void VirtualBrushGrid::UpdateLayout() {
	int width = GetClientSize().x;
	if (width <= 0) {
		width = 200; // Default
	}

	if (display_mode == BRUSHLIST_LISTBOX || display_mode == BRUSHLIST_TEXT_LISTBOX) {
		columns = 1;
		item_size = 32; // Fixed height for list items
	} else {
		// Icon modes
		if (icon_size == RENDER_SIZE_16x16) {
			item_size = 18;
		} else {
			item_size = 34; // 32 + border
		}
		columns = std::max(1, (width - padding) / (item_size + padding));
	}

	int rows = (static_cast<int>(tileset->size()) + columns - 1) / columns;
	int contentHeight = rows * (item_size + padding) + padding;

	UpdateScrollbar(contentHeight);
}

wxSize VirtualBrushGrid::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

int VirtualBrushGrid::GetOrCreateBrushTexture(NVGcontext* vg, Brush* brush) {
	if (!brush) {
		return 0;
	}

	// Use brush pointer address as unique ID (stable during runtime)
	uint32_t brushId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(brush) & 0xFFFFFFFF);

	// Check cache first
	int existingTex = GetCachedImage(brushId);
	if (existingTex > 0) {
		return existingTex;
	}

	// Get sprite RGBA data
	Sprite* spr = brush->getSprite();
	if (!spr) {
		spr = g_gui.gfx.getSprite(brush->getLookID());
	}
	if (!spr) {
		return 0;
	}

	// Try to get as GameSprite for RGBA access
	GameSprite* gs = dynamic_cast<GameSprite*>(spr);
	if (!gs || gs->spriteList.empty()) {
		return 0;
	}

	// Calculate composite size
	int w = gs->width * 32;
	int h = gs->height * 32;
	if (w <= 0 || h <= 0) {
		return 0;
	}

	// Create composite RGBA buffer
	size_t bufferSize = static_cast<size_t>(w) * h * 4;
	std::vector<uint8_t> composite(bufferSize, 0);

	// Composite all layers
	int px = (gs->pattern_x >= 3) ? 2 : 0;
	for (int l = 0; l < gs->layers; ++l) {
		for (int sw = 0; sw < gs->width; ++sw) {
			for (int sh = 0; sh < gs->height; ++sh) {
				int idx = gs->getIndex(sw, sh, l, px, 0, 0, 0);
				if (idx < 0 || static_cast<size_t>(idx) >= gs->spriteList.size()) {
					continue;
				}

				auto data = gs->spriteList[idx]->getRGBAData();
				if (!data) {
					continue;
				}

				int part_x = (gs->width - sw - 1) * 32;
				int part_y = (gs->height - sh - 1) * 32;

				for (int sy = 0; sy < 32; ++sy) {
					for (int sx = 0; sx < 32; ++sx) {
						int dy = part_y + sy;
						int dx = part_x + sx;
						int di = (dy * w + dx) * 4;
						int si = (sy * 32 + sx) * 4;

						uint8_t sa = data[si + 3];
						if (sa == 0) {
							continue;
						}

						if (sa == 255) {
							composite[di + 0] = data[si + 0];
							composite[di + 1] = data[si + 1];
							composite[di + 2] = data[si + 2];
							composite[di + 3] = 255;
						} else {
							float a = sa / 255.0f;
							float ia = 1.0f - a;
							composite[di + 0] = static_cast<uint8_t>(data[si + 0] * a + composite[di + 0] * ia);
							composite[di + 1] = static_cast<uint8_t>(data[si + 1] * a + composite[di + 1] * ia);
							composite[di + 2] = static_cast<uint8_t>(data[si + 2] * a + composite[di + 2] * ia);
							composite[di + 3] = std::max(composite[di + 3], sa);
						}
					}
				}
			}
		}
	}

	// Create NanoVG image
	return GetOrCreateImage(brushId, composite.data(), w, h);
}

void VirtualBrushGrid::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Update layout if needed
	int newCols = std::max(1, (width - padding) / (item_size + padding));
	if (newCols != columns) {
		columns = newCols;
		int rows = (static_cast<int>(tileset->size()) + columns - 1) / columns;
		int contentHeight = rows * (item_size + padding) + padding;
		UpdateScrollbar(contentHeight);
	}

	// Calculate visible range
	int scrollPos = GetScrollPosition();
	int rowHeight = item_size + padding;
	int startRow = scrollPos / rowHeight;
	int endRow = (scrollPos + height + rowHeight - 1) / rowHeight + 1;

	int startIdx = startRow * columns;
	int endIdx = std::min(static_cast<int>(tileset->size()), endRow * columns);

	// Draw visible items
	for (int i = startIdx; i < endIdx; ++i) {
		DrawBrushItem(vg, i, GetItemRect(i));
	}
}

void VirtualBrushGrid::DrawBrushItem(NVGcontext* vg, int i, const wxRect& rect) {
	float x = static_cast<float>(rect.x);
	float y = static_cast<float>(rect.y);
	float w = static_cast<float>(rect.width);
	float h = static_cast<float>(rect.height);

	// Background and Highlights
	nvgBeginPath(vg);
	if (display_mode == BRUSHLIST_LISTBOX || display_mode == BRUSHLIST_TEXT_LISTBOX) {
		nvgRect(vg, x, y, w, h);
	} else {
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
	}

	if (i == selected_index) {
		nvgFillColor(vg, nvgRGBA(80, 100, 120, 255));
	} else if (i == hover_index) {
		nvgFillColor(vg, nvgRGBA(70, 70, 75, 255));
	} else {
		if (display_mode == BRUSHLIST_LISTBOX || display_mode == BRUSHLIST_TEXT_LISTBOX) {
			// Alternating row colors could go here, but flat is fine for now
			nvgFillColor(vg, nvgRGBA(50, 50, 55, 255));
		} else {
			NVGpaint bgPaint = nvgLinearGradient(vg, x, y, x, y + h, nvgRGBA(60, 60, 65, 255), nvgRGBA(50, 50, 55, 255));
			nvgFillPaint(vg, bgPaint);
		}
	}
	nvgFill(vg);

	// Selection Border for Icons
	if (i == selected_index && (display_mode == BRUSHLIST_SMALL_ICONS || display_mode == BRUSHLIST_LARGE_ICONS)) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 4.0f);
		nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	Brush* brush = (i < static_cast<int>(tileset->size())) ? tileset->brushlist[i] : nullptr;
	if (!brush) {
		return;
	}

	// Content
	if (display_mode == BRUSHLIST_TEXT_LISTBOX) {
		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgText(vg, x + 10, y + h * 0.5f, brush->getName().c_str(), nullptr);
	} else if (display_mode == BRUSHLIST_LISTBOX) {
		// Icon + Text
		int tex = GetOrCreateBrushTexture(vg, brush);
		if (tex > 0) {
			float iconSize = h - 4;
			NVGpaint imgPaint = nvgImagePattern(vg, x + 2, y + 2, iconSize, iconSize, 0.0f, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, x + 2, y + 2, iconSize, iconSize);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		nvgFontSize(vg, 16.0f);
		nvgFontFace(vg, "sans");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgText(vg, x + h + 10, y + h * 0.5f, brush->getName().c_str(), nullptr);
	} else {
		// Icons Only
		int tex = GetOrCreateBrushTexture(vg, brush);
		if (tex > 0) {
			int iconSize = item_size - 4;
			int iconX = rect.x + 2;
			int iconY = rect.y + 2;

			NVGpaint imgPaint = nvgImagePattern(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 0.0f, tex, 1.0f);

			nvgBeginPath(vg);
			nvgRoundedRect(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 3.0f);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}
	}
}

wxRect VirtualBrushGrid::GetItemRect(int index) const {
	int row = index / columns;
	int col = index % columns;

	if (display_mode == BRUSHLIST_LISTBOX || display_mode == BRUSHLIST_TEXT_LISTBOX) {
		int width = GetClientSize().x;
		// Ensure width is valid (might be called before layout)
		if (width <= 0) width = 200;
		return wxRect(
			padding,
			padding + row * (item_size + padding),
			width - 2 * padding,
			item_size
		);
	}

	return wxRect(
		padding + col * (item_size + padding),
		padding + row * (item_size + padding),
		item_size,
		item_size
	);
}

int VirtualBrushGrid::HitTest(int x, int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int realX = x;

	int row = (realY - padding) / (item_size + padding);
	int col = 0;

	if (display_mode != BRUSHLIST_LISTBOX && display_mode != BRUSHLIST_TEXT_LISTBOX) {
		col = (realX - padding) / (item_size + padding);
		if (col < 0 || col >= columns) {
			return -1;
		}
	}

	if (row < 0) {
		return -1;
	}

	int index = row * columns + col;
	if (index >= 0 && index < static_cast<int>(tileset->size())) {
		// Use GetItemRect for precise hit testing (especially width)
		wxRect rect = GetItemRect(index);
		rect.y -= scrollPos;
		if (rect.Contains(x, y)) {
			return index;
		}
	}

	return -1;
}

void VirtualBrushGrid::OnMouseDown(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());
	if (index != -1 && index != selected_index) {
		selected_index = index;

		// Notify GUI - find PaletteWindow parent
		wxWindow* w = GetParent();
		while (w) {
			PaletteWindow* pw = dynamic_cast<PaletteWindow*>(w);
			if (pw) {
				g_gui.ActivatePalette(pw);
				break;
			}
			w = w->GetParent();
		}

		g_gui.SelectBrush(tileset->brushlist[selected_index], tileset->getType());
		Refresh();
	}
}

void VirtualBrushGrid::OnMotion(wxMouseEvent& event) {
	int index = HitTest(event.GetX(), event.GetY());

	if (index != hover_index) {
		hover_index = index;
		Refresh();
	}

	// Tooltip
	if (index != -1) {
		Brush* brush = tileset->brushlist[index];
		if (brush) {
			wxString tip = wxstr(brush->getName());
			if (GetToolTipText() != tip) {
				SetToolTip(tip);
			}
		}
	} else {
		UnsetToolTip();
	}

	event.Skip();
}

void VirtualBrushGrid::OnTimer(wxTimerEvent& event) {
	// Animation tick for hover effects (optional - can be enhanced later)
	Refresh();
}

void VirtualBrushGrid::SelectFirstBrush() {
	if (tileset->size() > 0) {
		selected_index = 0;
		Refresh();
	}
}

Brush* VirtualBrushGrid::GetSelectedBrush() const {
	if (selected_index >= 0 && selected_index < static_cast<int>(tileset->size())) {
		return tileset->brushlist[selected_index];
	}
	return nullptr;
}

bool VirtualBrushGrid::SelectBrush(const Brush* brush) {
	for (size_t i = 0; i < tileset->size(); ++i) {
		if (tileset->brushlist[i] == brush) {
			selected_index = static_cast<int>(i);

			// Ensure visible
			wxRect rect = GetItemRect(selected_index);
			int scrollPos = GetScrollPosition();
			int clientHeight = GetClientSize().y;

			if (rect.y < scrollPos) {
				SetScrollPosition(rect.y - padding);
			} else if (rect.y + rect.height > scrollPos + clientHeight) {
				SetScrollPosition(rect.y + rect.height - clientHeight + padding);
			}

			Refresh();
			return true;
		}
	}
	selected_index = -1;
	Refresh();
	return false;
}
