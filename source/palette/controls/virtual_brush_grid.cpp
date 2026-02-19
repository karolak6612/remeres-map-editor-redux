#include "app/main.h"
#include "palette/controls/virtual_brush_grid.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"

#include <glad/glad.h>

#include <nanovg.h>
#include <nanovg_gl.h>

#include "util/nvg_utils.h"
#include "ui/theme.h"

#include <spdlog/spdlog.h>

VirtualBrushGrid::VirtualBrushGrid(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz) :
	NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS),
	BrushBoxInterface(_tileset),
	icon_size(rsz),
	selected_index(-1),
	hover_index(-1),
	columns(1),
	item_size(0),
	padding(4),
	m_animTimer(this) {

	// Start animation timer (60fps)
	m_animTimer.Start(16);

	if (icon_size == RENDER_SIZE_16x16) {
		item_size = 18;
	} else {
		item_size = GRID_ITEM_SIZE_BASE + 2; // + borders
	}

	Bind(wxEVT_LEFT_DOWN, &VirtualBrushGrid::OnMouseDown, this);
	Bind(wxEVT_MOTION, &VirtualBrushGrid::OnMotion, this);
	Bind(wxEVT_SIZE, &VirtualBrushGrid::OnSize, this);
	Bind(wxEVT_TIMER, &VirtualBrushGrid::OnTimer, this);

	UpdateLayout();
}

VirtualBrushGrid::~VirtualBrushGrid() = default;

void VirtualBrushGrid::SetDisplayMode(DisplayMode mode) {
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

	if (display_mode == DisplayMode::List) {
		columns = 1;
		int rows = static_cast<int>(tileset->size());
		int contentHeight = rows * LIST_ROW_HEIGHT + padding;
		UpdateScrollbar(contentHeight);
	} else {
		columns = std::max(1, (width - padding) / (item_size + padding));
		int rows = (static_cast<int>(tileset->size()) + columns - 1) / columns;
		int contentHeight = rows * (item_size + padding) + padding;
		UpdateScrollbar(contentHeight);
	}
}

wxSize VirtualBrushGrid::DoGetBestClientSize() const {
	return FromDIP(wxSize(200, 300));
}

void VirtualBrushGrid::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Calculate visible range
	int scrollPos = GetScrollPosition();
	int rowHeight = (display_mode == DisplayMode::List) ? LIST_ROW_HEIGHT : (item_size + padding);
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

	bool isSelected = (i == selected_index);
	bool isHovered = (i == hover_index);

	// 1. Card Shadows
	// Offset dark transparent rect behind card for depth
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 2, y + 2, w, h, 4.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 60)); // 2px offset, 50% opacity black equivalent
	nvgFill(vg);

	// 2. Glow on Hover (Double-render technique)
	if (isHovered || (isSelected && m_selectionAnim > 0.01f)) {
		float glowAlpha = isSelected ? m_selectionAnim : m_hoverAnim;
		if (glowAlpha > 0.0f) {
			// Larger blurred pass
			NVGpaint glowPaint = nvgBoxGradient(vg, x, y, w, h, 8.0f, 12.0f, nvgRGBA(100, 150, 255, (unsigned char)(100 * glowAlpha)), nvgRGBA(0, 0, 0, 0));
			nvgBeginPath(vg);
			nvgRect(vg, x - 15, y - 15, w + 30, h + 30);
			nvgRoundedRect(vg, x, y, w, h, 4.0f);
			nvgPathWinding(vg, NVG_HOLE);
			nvgFillPaint(vg, glowPaint);
			nvgFill(vg);

			// Crisp pass (inner glow)
			if (isSelected) {
				nvgBeginPath(vg);
				nvgRoundedRect(vg, x - 1, y - 1, w + 2, h + 2, 5.0f);
				nvgStrokeColor(vg, nvgRGBA(100, 150, 255, (unsigned char)(180 * glowAlpha)));
				nvgStrokeWidth(vg, 1.5f);
				nvgStroke(vg);
			}
		}
	}

	// 3. Card Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4.0f);

	if (isSelected) {
		NVGcolor selCol = NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent));
		selCol.a = 1.0f; // Force opaque
		nvgFillColor(vg, selCol);
	} else if (isHovered) {
		// Use alpha blend for hover transition
		NVGcolor base = NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBase));
		NVGcolor hover = NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBaseHover));
		// Simple lerp based on m_hoverAnim
		NVGcolor finalCol;
		finalCol.r = base.r + (hover.r - base.r) * m_hoverAnim;
		finalCol.g = base.g + (hover.g - base.g) * m_hoverAnim;
		finalCol.b = base.b + (hover.b - base.b) * m_hoverAnim;
		finalCol.a = 1.0f;
		nvgFillColor(vg, finalCol);
	} else {
		nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::CardBase)));
	}
	nvgFill(vg);

	// 4. Smooth Selection Border
	if (isSelected) {
		float strokeAlpha = m_selectionAnim; // 0.0 to 1.0
		float strokeW = 1.0f + (1.0f * m_selectionAnim); // Animate width 1px -> 2px

		nvgBeginPath(vg);
		nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 4.0f);
		NVGcolor borderCol = NvgUtils::ToNvColor(Theme::Get(Theme::Role::Accent));
		borderCol.a = strokeAlpha;
		nvgStrokeColor(vg, borderCol);
		nvgStrokeWidth(vg, strokeW);
		nvgStroke(vg);
	}

	// Draw brush sprite
	Brush* brush = (i < static_cast<int>(tileset->size())) ? tileset->brushlist[i] : nullptr;
	if (brush) {
		Sprite* spr = brush->getSprite();
		if (!spr) {
			spr = g_gui.gfx.getSprite(brush->getLookID());
		}

		if (!spr) {
			return; // Safety check
		}

		int tex = GetOrCreateSpriteTexture(vg, spr);
		if (tex > 0) {
			int iconSize = (display_mode == DisplayMode::List) ? GRID_ITEM_SIZE_BASE : (item_size - 2 * ICON_OFFSET);
			int iconX = rect.x + ICON_OFFSET;
			int iconY = rect.y + ICON_OFFSET;

			NVGpaint imgPaint = nvgImagePattern(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 0.0f, tex, 1.0f);

			nvgBeginPath(vg);
			nvgRoundedRect(vg, static_cast<float>(iconX), static_cast<float>(iconY), static_cast<float>(iconSize), static_cast<float>(iconSize), 3.0f);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		}

		if (display_mode == DisplayMode::List) {
			nvgFontSize(vg, 14.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, NvgUtils::ToNvColor(Theme::Get(Theme::Role::Text)));

			auto it = m_utf8NameCache.find(brush);
			if (it == m_utf8NameCache.end()) {
				m_utf8NameCache[brush] = std::string(wxstr(brush->getName()).ToUTF8());
				it = m_utf8NameCache.find(brush);
			}
			nvgText(vg, rect.x + 40, rect.y + rect.height / 2.0f, it->second.c_str(), nullptr);
		}
	}
}

wxRect VirtualBrushGrid::GetItemRect(int index) const {
	if (display_mode == DisplayMode::List) {
		int width = GetClientSize().x - 2 * padding;
		return wxRect(padding, padding + index * LIST_ROW_HEIGHT, width, LIST_ROW_HEIGHT);
	} else {
		int row = index / columns;
		int col = index % columns;

		return wxRect(
			padding + col * (item_size + padding),
			padding + row * (item_size + padding),
			item_size,
			item_size
		);
	}
}

int VirtualBrushGrid::HitTest(int x, int y) const {
	int scrollPos = GetScrollPosition();
	int realY = y + scrollPos;
	int realX = x;

	if (display_mode == DisplayMode::List) {
		int row = (realY - padding) / LIST_ROW_HEIGHT;

		if (row < 0 || row >= static_cast<int>(tileset->size())) {
			return -1;
		}

		int index = row;
		// Check horizontal bounds properly
		int width = GetClientSize().x - 2 * padding;
		if (realX >= padding && realX <= GetClientSize().x - padding) {
			return index;
		}
		return -1;
	} else {
		int col = (realX - padding) / (item_size + padding);
		int row = (realY - padding) / (item_size + padding);

		if (col < 0 || col >= columns || row < 0) {
			return -1;
		}

		int index = row * columns + col;
		if (index >= 0 && index < static_cast<int>(tileset->size())) {
			wxRect rect = GetItemRect(index);
			// Adjust rect to scroll position for contains check
			rect.y -= scrollPos;
			if (rect.Contains(x, y)) {
				return index;
			}
		}
		return -1;
	}
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
		// Reset hover animation to 0.5 to give a quick pulse up to 1.0
		if (hover_index != -1) m_hoverAnim = 0.5f;
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
	bool dirty = false;

	// Animate Hover
	float targetHover = (hover_index != -1) ? 1.0f : 0.0f;
	// If hovering a different item, reset animation for quick feedback or handle per-item anims.
	// For now, global hover anim is simple.
	if (std::abs(m_hoverAnim - targetHover) > 0.01f) {
		m_hoverAnim += (targetHover - m_hoverAnim) * 0.2f;
		dirty = true;
	}

	// Animate Selection
	float targetSelection = (selected_index != -1) ? 1.0f : 0.0f;
	if (std::abs(m_selectionAnim - targetSelection) > 0.01f) {
		m_selectionAnim += (targetSelection - m_selectionAnim) * 0.15f;
		dirty = true;
	}

	if (dirty) {
		Refresh();
	}
}

void VirtualBrushGrid::OnSize(wxSizeEvent& event) {
	UpdateLayout();
	Refresh();
	event.Skip();
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
