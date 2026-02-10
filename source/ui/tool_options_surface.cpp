#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "ui/artprovider.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include "util/nvg_utils.h"

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"

#include <nanovg.h>

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxBORDER_NONE | wxWANTS_CHARS) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
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
	// Base padding
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
	int slider_w = w - FromDIP(8);

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
	}

	InvalidateBestSize();
}

void ToolOptionsSurface::OnNanoVGPaint(NVGcontext* vg, int width, int height) {
	// Background
	// NVG background is cleared by NanoVGCanvas::OnPaint

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

	wxRect r = tr.rect;
	float x = (float)r.x;
	float y = (float)r.y;
	float w = (float)r.width;
	float h = (float)r.height;

	// Background
	if (is_selected) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(50, 100, 200, 200)); // Highlight
		nvgFill(vg);

		// Glow
		NVGpaint glow = nvgBoxGradient(vg, x, y, w, h, 4.0f, 8.0f, nvgRGBA(50, 100, 255, 128), nvgRGBA(0,0,0,0));
		nvgBeginPath(vg);
		nvgRect(vg, x-10, y-10, w+20, h+20);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, glow);
		nvgFill(vg);

	} else if (is_hover) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(200, 200, 200, 50));
		nvgFill(vg);
	}

	// Draw Brush Sprite
	if (tr.brush) {
		int tex = 0;
		// Use memory address as ID
		uintptr_t ptr = reinterpret_cast<uintptr_t>(tr.brush);
		uint32_t brushId = static_cast<uint32_t>(ptr ^ (ptr >> 32));

		tex = GetCachedImage(brushId);
		if (tex == 0) {
			Sprite* s = tr.brush->getSprite();
			if (!s && tr.brush->getLookID() != 0) {
				s = g_gui.gfx.getSprite(tr.brush->getLookID());
			}
			if (s) {
				GameSprite* gs = dynamic_cast<GameSprite*>(s);
				if (gs) {
					int iw, ih;
					// CreateCompositeRGBA allocates memory, we need to pass it to GetOrCreateImage
					// NvgUtils::CreateCompositeRGBA returns std::unique_ptr<uint8_t[]>
					auto data = NvgUtils::CreateCompositeRGBA(*gs, iw, ih);
					if (data) {
						tex = GetOrCreateImage(brushId, data.get(), iw, ih);
					}
				}
			}
		}

		if (tex > 0) {
			int iconSize = 32;
			float ix = x + (w - iconSize) / 2;
			float iy = y + (h - iconSize) / 2;
			NVGpaint imgPaint = nvgImagePattern(vg, ix, iy, (float)iconSize, (float)iconSize, 0.0f, tex, 1.0f);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, ix, iy, (float)iconSize, (float)iconSize, 2.0f);
			nvgFillPaint(vg, imgPaint);
			nvgFill(vg);
		} else {
			// Fallback text
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, x + w/2, y + h/2, tr.tooltip.Left(1).ToStdString().c_str(), nullptr);
		}
	}

	// Border
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 4.0f);
	if (is_selected) {
		nvgStrokeColor(vg, nvgRGBA(100, 150, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
	} else {
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 128));
		nvgStrokeWidth(vg, 1.0f);
	}
	nvgStroke(vg);
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const wxString& label, int value, int min, int max, bool active) {
	float x = (float)rect.x;
	float y = (float)rect.y;
	float w = (float)rect.width;
	float h = (float)rect.height;

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255)); // Assume light theme or adaptive?
	// Actually text color should depend on theme. Using black/dark grey for now.
	// RME seems to use light grey backgrounds.

	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, x, y + h/2, label.ToStdString().c_str(), nullptr);

	// Measure label to position track
	float bounds[4];
	nvgTextBounds(vg, x, y, label.ToStdString().c_str(), nullptr, bounds);
	float labelW = bounds[2] - bounds[0] + 10.0f;
	if (labelW < FromDIP(70)) labelW = FromDIP(70);

	// Track
	float tx = x + labelW;
	float tw = w - labelW - FromDIP(40); // space for value
	float th = 4.0f;
	float ty = y + (h - th) / 2;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, tx, ty, tw, th, 2.0f);
	nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
	nvgFill(vg);

	// Fill
	if (value > min && max > min) {
		float pct = (float)(value - min) / (float)(max - min);
		pct = std::clamp(pct, 0.0f, 1.0f);
		float fw = tw * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, tx, ty, fw, th, 2.0f);
		nvgFillColor(vg, nvgRGBA(50, 150, 250, 255));
		nvgFill(vg);

		// Thumb
		nvgBeginPath(vg);
		nvgCircle(vg, tx + fw, ty + th/2, 6.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, tx + fw, ty + th/2, 6.0f);
		nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Value
	wxString valStr = wxString::Format("%d", value);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, tx + tw + 10, y + h/2, valStr.ToStdString().c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const wxString& label, bool value, bool hover) {
	float x = (float)rect.x;
	float y = (float)rect.y;
	float h = (float)rect.height;

	float boxSz = 14.0f;
	float boxY = y + (h - boxSz) / 2;

	// Box
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, boxY, boxSz, boxSz, 3.0f);
	if (value) {
		nvgFillColor(vg, nvgRGBA(50, 150, 250, 255));
		nvgFill(vg);
	} else {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgStrokeColor(vg, hover ? nvgRGBA(50, 150, 250, 255) : nvgRGBA(150, 150, 150, 255));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Check
	if (value) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, x + 3, boxY + 7);
		nvgLineTo(vg, x + 6, boxY + 10);
		nvgLineTo(vg, x + 11, boxY + 3);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, x + boxSz + 8.0f, y + h/2, label.ToStdString().c_str(), nullptr);
}

void ToolOptionsSurface::OnMouse(wxMouseEvent& evt) {
	m_hoverPos = evt.GetPosition();

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
			// Tooltip handling could go here (delayed)
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
			// Calculate value
			// In a real impl, refactor the math to a helper
			int label_w = FromDIP(70);
			int track_x = interactables.size_slider_rect.GetLeft() + label_w;
			int track_w = interactables.size_slider_rect.width - label_w - FromDIP(40);

			int rel_x = m_hoverPos.x - track_x;
			float pct = (float)rel_x / (float)track_w;
			if (pct < 0) {
				pct = 0;
			}
			if (pct > 1) {
				pct = 1;
			}

			int min = 1, max = 15;
			int val = min + (int)(pct * (max - min));
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size); // Assuming square for now
				Refresh();
			}
		}
		// Thickness similar...
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
