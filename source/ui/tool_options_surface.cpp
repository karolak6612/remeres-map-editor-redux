#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "ui/artprovider.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include "rendering/utilities/sprite_icon_generator.h"

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"

#include <nanovg.h>
#include <format>

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxVSCROLL | wxWANTS_CHARS) {
	m_animTimer.SetOwner(this);

	Bind(wxEVT_LEFT_DOWN, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_UP, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_MOTION, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &ToolOptionsSurface::OnLeave, this);
	Bind(wxEVT_SIZE, &ToolOptionsSurface::OnSize, this);
	Bind(wxEVT_TIMER, &ToolOptionsSurface::OnTimer, this);
}

ToolOptionsSurface::~ToolOptionsSurface() {
}

wxSize ToolOptionsSurface::DoGetBestClientSize() const {
	// Calculate height based on current layout
	int h = FromDIP(8);

	// Tools Section
	if (!tool_rects.empty()) {
		int max_y = 0;
		for (const auto& tr : tool_rects) {
			max_y = std::max(max_y, tr.rect.GetBottom());
		}
		h = max_y + FromDIP(SECTION_GAP);
	}

	// Sliders Section
	if (interactables.size_slider_rect.height > 0) {
		h += interactables.size_slider_rect.height + FromDIP(GRID_GAP);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		h += interactables.thickness_slider_rect.height + FromDIP(SECTION_GAP);
	}

	// Checkboxes
	if (interactables.preview_check_rect.height > 0) {
		h += interactables.preview_check_rect.height + FromDIP(4);
	}
	if (interactables.lock_check_rect.height > 0) {
		h += interactables.lock_check_rect.height + FromDIP(4);
	}

	// Min width 200 DIP?
	return wxSize(FromDIP(240), h + FromDIP(8));
}

void ToolOptionsSurface::DoSetSizeHints(int minW, int minH, int maxW, int maxH, int incW, int incH) {
	wxControl::DoSetSizeHints(minW, minH, maxW, maxH, incW, incH);
}

void ToolOptionsSurface::RebuildLayout() {
	tool_rects.clear();
	interactables.size_slider_rect = wxRect();
	interactables.thickness_slider_rect = wxRect();
	interactables.preview_check_rect = wxRect();
	interactables.lock_check_rect = wxRect();

	if (current_type == TILESET_UNKNOWN) {
		UpdateScrollbar(0);
		return;
	}

	int x = FromDIP(4);
	int y = FromDIP(4);
	const int w = GetClientSize().GetWidth();
	const int icon_sz = FromDIP(ICON_SIZE_LG); // Defaulting to large for "Pro" look
	const int gap = FromDIP(GRID_GAP);

	// 1. Tools
	bool has_tools = (current_type == TILESET_TERRAIN || current_type == TILESET_COLLECTION);

	if (has_tools) {
		// Populate tool list based on BrushManager
		std::vector<Brush*> brushes;

		if (g_brush_manager.optional_brush) {
			brushes.push_back(g_brush_manager.optional_brush);
		}
		if (g_brush_manager.eraser) {
			brushes.push_back(g_brush_manager.eraser);
		}
		if (g_brush_manager.pz_brush) {
			brushes.push_back(g_brush_manager.pz_brush);
		}
		if (g_brush_manager.rook_brush) {
			brushes.push_back(g_brush_manager.rook_brush);
		}
		if (g_brush_manager.nolog_brush) {
			brushes.push_back(g_brush_manager.nolog_brush);
		}
		if (g_brush_manager.pvp_brush) {
			brushes.push_back(g_brush_manager.pvp_brush);
		}

		// Doors?
		if (g_brush_manager.normal_door_brush) {
			brushes.push_back(g_brush_manager.normal_door_brush);
		}
		if (g_brush_manager.locked_door_brush) {
			brushes.push_back(g_brush_manager.locked_door_brush);
		}
		if (g_brush_manager.magic_door_brush) {
			brushes.push_back(g_brush_manager.magic_door_brush);
		}
		if (g_brush_manager.quest_door_brush) {
			brushes.push_back(g_brush_manager.quest_door_brush);
		}
		if (g_brush_manager.hatch_door_brush) {
			brushes.push_back(g_brush_manager.hatch_door_brush);
		}
		if (g_brush_manager.window_door_brush) {
			brushes.push_back(g_brush_manager.window_door_brush);
		}
		if (g_brush_manager.archway_door_brush) {
			brushes.push_back(g_brush_manager.archway_door_brush);
		}

		// Layout grid
		int cur_x = x;
		for (Brush* b : brushes) {
			if (cur_x + icon_sz > w) {
				cur_x = x;
				y += icon_sz + gap;
			}

			ToolRect tr;
			tr.rect = wxRect(cur_x, y, icon_sz, icon_sz);
			tr.brush = b;
			tr.tooltip = b->getName(); // Or specific label
			tool_rects.push_back(tr);

			cur_x += icon_sz + gap;
		}
		if (!brushes.empty()) {
			y += icon_sz + FromDIP(SECTION_GAP);
		}
	}

	int slider_h = FromDIP(24);
	int slider_w = std::max(10, w - FromDIP(8));

	// 2. Size Slider
	bool show_size = (current_type != TILESET_UNKNOWN); // Most show size
	if (show_size) {
		interactables.size_slider_rect = wxRect(x, y, slider_w, slider_h);
		y += slider_h + gap;
	}

	// 3. Thickness Slider
	bool show_thickness = (current_type == TILESET_COLLECTION || current_type == TILESET_DOODAD);
	if (show_thickness) {
		interactables.thickness_slider_rect = wxRect(x, y, slider_w, slider_h);
		y += slider_h + gap;
	}

	// 4. Options
	y += FromDIP(8); // Extra spacer
	if (has_tools) { // Assume terrain
		interactables.preview_check_rect = wxRect(x, y, slider_w, FromDIP(20));
		y += FromDIP(24);
		interactables.lock_check_rect = wxRect(x, y, slider_w, FromDIP(20));
		y += FromDIP(24);
	}

	UpdateScrollbar(y + FromDIP(8));
	InvalidateBestSize();
}

int ToolOptionsSurface::GetOrCreateBrushImage(NVGcontext* vg, Brush* brush) {
	if (!brush) return 0;

	// Use brush ID or address as key? Address is safer if ID is not unique or persistent.
	// But image cache uses uint32_t id. Brush ID should be unique.
	// Wait, Brush ID might be 0 for some brushes?
	// Using pointer address cast to uint32_t is risky on 64-bit.
	// Let's use lookID if available, or hash of name?
	// Different brushes might share lookID (same sprite). That's actually fine to share image.

	int lookID = brush->getLookID();
	if (lookID == 0) return 0;

	uint32_t cacheKey = static_cast<uint32_t>(lookID);

	int existing = GetCachedImage(cacheKey);
	if (existing > 0) return existing;

	GameSprite* spr = g_gui.gfx.getSprite(lookID);
	if (!spr) return 0;

	wxBitmap bmp = SpriteIconGenerator::Generate(spr, SPRITE_SIZE_32x32);
	if (!bmp.IsOk()) return 0;

	wxImage img = bmp.ConvertToImage();
	if (!img.IsOk()) return 0;

	// Convert to RGBA
	int w = img.GetWidth();
	int h = img.GetHeight();
	std::vector<uint8_t> rgba(w * h * 4);
	const uint8_t* data = img.GetData();
	const uint8_t* alpha = img.GetAlpha();

	for (int i = 0; i < w * h; ++i) {
		rgba[i * 4 + 0] = data[i * 3 + 0];
		rgba[i * 4 + 1] = data[i * 3 + 1];
		rgba[i * 4 + 2] = data[i * 3 + 2];
		if (alpha) {
			rgba[i * 4 + 3] = alpha[i];
		} else {
			rgba[i * 4 + 3] = 255;
		}
	}

	return GetOrCreateImage(cacheKey, rgba.data(), w, h);
}

void ToolOptionsSurface::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Background filled by NanoVGCanvas or we do it?
	// NanoVGCanvas doesn't automatically clear to specific color in OnNanoVGPaint unless we do it.
	// But it sets up the frame.

	// Draw background
	nvgBeginPath(vg);
	nvgRect(vg, 0, GetScrollPosition(), width, height); // Fill visible area
	nvgFillColor(vg, nvgRGBA(240, 240, 240, 255)); // Light grey background
	nvgFill(vg);

	// 1. Draw Tools
	for (const auto& tr : tool_rects) {
		DrawToolIcon(vg, tr);
	}

	// 2. Draw Sliders
	if (interactables.size_slider_rect.height > 0) {
		DrawSlider(vg, interactables.size_slider_rect, "Size", current_size, 1, 15, true);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		DrawSlider(vg, interactables.thickness_slider_rect, "Thickness", current_thickness, 1, 100, true);
	}

	// 3. Checkboxes
	if (interactables.preview_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.preview_check_rect, "Preview Border", show_preview, interactables.hover_preview);
	}
	if (interactables.lock_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.lock_check_rect, "Lock Doors (Shift)", lock_doors, interactables.hover_lock);
	}
}

void ToolOptionsSurface::DrawToolIcon(NVGcontext* vg, const ToolRect& tr) {
	bool is_selected = (active_brush == tr.brush);
	bool is_hover = (hover_brush == tr.brush);

	float x = tr.rect.x;
	float y = tr.rect.y;
	float w = tr.rect.width;
	float h = tr.rect.height;

	// Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4.0f);
	if (is_selected) {
		nvgFillColor(vg, nvgRGBA(200, 220, 255, 255)); // Highlight
	} else if (is_hover) {
		nvgFillColor(vg, nvgRGBA(220, 220, 220, 255)); // Hover
	} else {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // Normal
	}
	nvgFill(vg);

	// Border
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 4.0f);
	if (is_selected) {
		nvgStrokeColor(vg, nvgRGBA(50, 100, 200, 255));
		nvgStrokeWidth(vg, 2.0f);
	} else {
		nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 255));
		nvgStrokeWidth(vg, 1.0f);
	}
	nvgStroke(vg);

	// Icon
	if (tr.brush) {
		int imgId = GetOrCreateBrushImage(vg, tr.brush);
		if (imgId > 0) {
			// Center 32x32 icon
			float ix = x + (w - 32) / 2;
			float iy = y + (h - 32) / 2;

			NVGpaint imgPaint = nvgImagePattern(vg, ix, iy, 32, 32, 0, imgId, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, ix, iy, 32, 32);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		} else {
			// Fallback text
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
			std::string label = tr.brush->getName().substr(0, 1);
			nvgText(vg, x + w/2, y + h/2, label.c_str(), nullptr);
		}
	}
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const wxString& label, int value, int min, int max, bool active) {
	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, rect.GetLeft(), rect.GetTop() + rect.height / 2, label.ToUTF8().data(), nullptr);

	// Track
	int label_w = FromDIP(70);
	float track_x = rect.GetLeft() + label_w;
	float track_w = rect.width - label_w - FromDIP(40);
	float track_h = FromDIP(4);
	float track_y = rect.GetTop() + (rect.height - track_h) / 2;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, track_x, track_y, track_w, track_h, 2.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgFill(vg);

	// Fill
	if (value > min && max > min) {
		float pct = (float)(value - min) / (float)(max - min);
		float fill_w = track_w * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, track_x, track_y, fill_w, track_h, 2.0f);
		nvgFillColor(vg, nvgRGBA(50, 100, 200, 255));
		nvgFill(vg);

		// Thumb
		nvgBeginPath(vg);
		nvgCircle(vg, track_x + fill_w, track_y + track_h / 2, FromDIP(5));
		nvgFillColor(vg, nvgRGBA(80, 80, 80, 255));
		nvgFill(vg);
	}

	// Value
	std::string valStr = std::to_string(value);
	nvgText(vg, track_x + track_w + FromDIP(8), rect.GetTop() + rect.height / 2, valStr.c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const wxString& label, bool value, bool hover) {
	float box_sz = FromDIP(14);
	float box_y = rect.GetTop() + (rect.height - box_sz) / 2;
	float box_x = rect.GetLeft();

	nvgBeginPath(vg);
	nvgRoundedRect(vg, box_x, box_y, box_sz, box_sz, 3.0f);
	if (value) {
		nvgFillColor(vg, nvgRGBA(50, 100, 200, 255));
		nvgFill(vg);

		// Checkmark
		nvgBeginPath(vg);
		nvgMoveTo(vg, box_x + 3, box_y + 7);
		nvgLineTo(vg, box_x + 6, box_y + 10);
		nvgLineTo(vg, box_x + box_sz - 3, box_y + 4);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	} else {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgStrokeColor(vg, hover ? nvgRGBA(50, 100, 200, 255) : nvgRGBA(150, 150, 150, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, box_x + box_sz + FromDIP(8), box_y + box_sz / 2, label.ToUTF8().data(), nullptr);
}

void ToolOptionsSurface::OnMouse(wxMouseEvent& evt) {
	// Adjust mouse position for scrolling
	m_hoverPos = evt.GetPosition();
	m_hoverPos.y += GetScrollPosition();

	// Hit testing
	Brush* prev_hover = hover_brush;
	hover_brush = nullptr;

	bool prev_preview = interactables.hover_preview;
	bool prev_lock = interactables.hover_lock;
	interactables.hover_preview = false;
	interactables.hover_lock = false;

	// Tools
	for (const auto& tr : tool_rects) {
		if (tr.rect.Contains(m_hoverPos)) {
			hover_brush = tr.brush;
			break;
		}
	}

	// Sliders Interaction
	if (evt.LeftDown()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_size = true;
			CaptureMouse();
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_thickness = true;
			CaptureMouse();
		} else if (interactables.preview_check_rect.Contains(m_hoverPos)) {
			show_preview = !show_preview;
			g_settings.setInteger(Config::SHOW_AUTOBORDER_PREVIEW, show_preview);
			Refresh();
		} else if (interactables.lock_check_rect.Contains(m_hoverPos)) {
			lock_doors = !lock_doors;
			g_settings.setInteger(Config::DRAW_LOCKED_DOOR, lock_doors);
			g_brush_manager.SetDoorLocked(lock_doors);
			Refresh();
		} else if (hover_brush) {
			SelectBrush(hover_brush);
		}
	}

	if (evt.Dragging() && evt.LeftIsDown()) {
		if (interactables.dragging_size) {
			int label_w = FromDIP(70);
			int track_x = interactables.size_slider_rect.GetLeft() + label_w;
			int track_w = interactables.size_slider_rect.width - label_w - FromDIP(40);

			int rel_x = m_hoverPos.x - track_x;
			float pct = (float)rel_x / (float)track_w;
			if (pct < 0) pct = 0;
			if (pct > 1) pct = 1;

			int min = 1, max = 15;
			int val = min + (int)(pct * (max - min));
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size);
				Refresh();
			}
		} else if (interactables.dragging_thickness) {
            int label_w = FromDIP(70);
			int track_x = interactables.thickness_slider_rect.GetLeft() + label_w;
			int track_w = interactables.thickness_slider_rect.width - label_w - FromDIP(40);

			int rel_x = m_hoverPos.x - track_x;
			float pct = (float)rel_x / (float)track_w;
			if (pct < 0) pct = 0;
			if (pct > 1) pct = 1;

            int min = 1, max = 100;
            int val = min + (int)(pct * (max - min));
            if (val != current_thickness) {
                current_thickness = val;
                g_brush_manager.SetBrushThickness(current_thickness, 100);
                Refresh();
            }
        }
	}

	if (evt.LeftUp()) {
		if (GetCapture() == this) {
			ReleaseMouse();
		}
		interactables.dragging_size = false;
		interactables.dragging_thickness = false;
	}

	// Hover states for checkboxes
	if (interactables.preview_check_rect.Contains(m_hoverPos)) {
		interactables.hover_preview = true;
	}
	if (interactables.lock_check_rect.Contains(m_hoverPos)) {
		interactables.hover_lock = true;
	}

	if (prev_hover != hover_brush || prev_preview != interactables.hover_preview || prev_lock != interactables.hover_lock) {
		Refresh();
	}
}

void ToolOptionsSurface::OnLeave(wxMouseEvent& evt) {
	hover_brush = nullptr;
	Refresh();
}

void ToolOptionsSurface::OnTimer(wxTimerEvent& evt) {
	// Anim logic if needed
}

void ToolOptionsSurface::OnSize(wxSizeEvent& evt) {
	RebuildLayout();
	Refresh();
	evt.Skip();
}

void ToolOptionsSurface::SetPaletteType(PaletteType type) {
	if (current_type == type) {
		return;
	}
	current_type = type;
	RebuildLayout();
	Refresh();
}

void ToolOptionsSurface::UpdateBrushSize(BrushShape shape, int size) {
	if (current_size != size) {
		current_size = size;
		Refresh();
	}
}

void ToolOptionsSurface::ReloadSettings() {
	show_preview = g_settings.getInteger(Config::SHOW_AUTOBORDER_PREVIEW);
	lock_doors = g_settings.getInteger(Config::DRAW_LOCKED_DOOR);
	RebuildLayout();
	Refresh();
}

void ToolOptionsSurface::SelectBrush(Brush* brush) {
	if (active_brush == brush) {
		return;
	}
	active_brush = brush;
	g_brush_manager.SelectBrush(brush);
	Refresh();
}
