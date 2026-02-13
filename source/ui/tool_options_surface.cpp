#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "rendering/core/graphics.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include <format>
#include <algorithm>
#include <cmath>

#include <glad/glad.h>
#include <nanovg.h>

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) : NanoVGCanvas(parent, wxID_ANY, wxWANTS_CHARS) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_animTimer.SetOwner(this);

	Bind(wxEVT_LEFT_DOWN, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_DCLICK, &ToolOptionsSurface::OnMouse, this);
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

	return wxSize(FromDIP(240), h + FromDIP(8));
}

void ToolOptionsSurface::DoSetSizeHints(int minW, int minH, int maxW, int maxH, int incW, int incH) {
	NanoVGCanvas::DoSetSizeHints(minW, minH, maxW, maxH, incW, incH);
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
	const int icon_sz = FromDIP(ICON_SIZE_LG);
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
			tr.tooltip = wxString::FromUTF8(b->getName());
			tr.hoverAnim = 0.0f;
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
	// 1. Draw Tools
	for (auto& tr : tool_rects) {
		DrawToolIcon(vg, tr);
	}

	// 2. Draw Sliders
	if (interactables.size_slider_rect.height > 0) {
		DrawSlider(vg, interactables.size_slider_rect, "Size", current_size, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE, true);
	}
	if (interactables.thickness_slider_rect.height > 0) {
		DrawSlider(vg, interactables.thickness_slider_rect, "Thickness", current_thickness, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS, true);
	}

	// 3. Checkboxes
	if (interactables.preview_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.preview_check_rect, "Preview Border", show_preview, interactables.hover_preview);
	}
	if (interactables.lock_check_rect.height > 0) {
		DrawCheckbox(vg, interactables.lock_check_rect, "Lock Doors (Shift)", lock_doors, interactables.hover_lock);
	}
}

void ToolOptionsSurface::DrawToolIcon(NVGcontext* vg, ToolRect& tr) {
	bool is_selected = (active_brush == tr.brush);
	bool is_hover = (hover_brush == tr.brush);

	// Animate hover state
	if (is_hover) {
		tr.hoverAnim = std::min(1.0f, tr.hoverAnim + 0.1f);
	} else {
		tr.hoverAnim = std::max(0.0f, tr.hoverAnim - 0.1f);
	}

	if (tr.hoverAnim > 0 || is_selected) {
		if (!m_animTimer.IsRunning()) m_animTimer.Start(16);
	}

	float x = tr.rect.x;
	float y = tr.rect.y;
	float w = tr.rect.width;
	float h = tr.rect.height;

	// Shadow / Glow (NanoVG style)
	if (is_selected) {
		NVGpaint shadowPaint = nvgBoxGradient(vg, x, y, w, h, 4.0f, 8.0f, nvgRGBA(0, 120, 215, 100), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgRect(vg, x - 5, y - 5, w + 10, h + 10);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, shadowPaint);
		nvgFill(vg);
	} else if (tr.hoverAnim > 0.01f) {
		NVGpaint shadowPaint = nvgBoxGradient(vg, x, y+2, w, h, 4.0f, 6.0f, nvgRGBA(0, 0, 0, 60 * tr.hoverAnim), nvgRGBA(0, 0, 0, 0));
		nvgBeginPath(vg);
		nvgRect(vg, x - 5, y - 5, w + 10, h + 10);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillPaint(vg, shadowPaint);
		nvgFill(vg);
	}

	// Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4.0f);
	if (is_selected) {
		nvgFillColor(vg, nvgRGBA(60, 60, 65, 255));
	} else {
		// Subtle gradient
		NVGpaint bgPaint = nvgLinearGradient(vg, x, y, x, y+h, nvgRGBA(50, 50, 55, 255), nvgRGBA(40, 40, 45, 255));
		nvgFillPaint(vg, bgPaint);
	}
	nvgFill(vg);

	// Hover overlay
	if (tr.hoverAnim > 0.01f) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 4.0f);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 30 * tr.hoverAnim));
		nvgFill(vg);
	}

	// Border
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1, h - 1, 4.0f);
	if (is_selected) {
		nvgStrokeColor(vg, nvgRGBA(0, 120, 215, 255));
		nvgStrokeWidth(vg, 2.0f);
	} else {
		nvgStrokeColor(vg, nvgRGBA(80, 80, 80, 255));
		nvgStrokeWidth(vg, 1.0f);
	}
	nvgStroke(vg);

	// Draw Brush Sprite
	if (tr.brush) {
		Sprite* s = tr.brush->getSprite();
		if (!s && tr.brush->getLookID() != 0) {
			s = g_gui.gfx.getSprite(tr.brush->getLookID());
		}

		if (s) {
			int tex = GetOrCreateSpriteTexture(vg, s);
			if (tex > 0) {
				float spriteSize = 32.0f; // Assuming standard icon size
				float iconX = x + (w - spriteSize) * 0.5f;
				float iconY = y + (h - spriteSize) * 0.5f;

				NVGpaint imgPaint = nvgImagePattern(vg, iconX, iconY, spriteSize, spriteSize, 0, tex, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, iconX, iconY, spriteSize, spriteSize);
				nvgFillPaint(vg, imgPaint);
				nvgFill(vg);
			}
		} else {
			// Fallback text
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
			nvgText(vg, x + w*0.5f, y + h*0.5f, tr.tooltip.Left(1).ToStdString().c_str(), nullptr);
		}
	}
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const wxString& label, int value, int min, int max, bool active) {
	float x = rect.x;
	float y = rect.y;
	float w = rect.width;
	float h = rect.height;

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, x, y + h * 0.5f, label.ToStdString().c_str(), nullptr);

	// Track
	float label_w = FromDIP(SLIDER_LABEL_WIDTH);
	float track_x = x + label_w;
	float track_w = w - label_w - FromDIP(SLIDER_TEXT_MARGIN);
	float track_h = FromDIP(4);
	float track_y = y + (h - track_h) * 0.5f;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, track_x, track_y, track_w, track_h, 2.0f);
	nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
	nvgFill(vg);

	// Fill
	if (value > min && max > min) {
		float pct = std::clamp(static_cast<float>(value - min) / static_cast<float>(max - min), 0.0f, 1.0f);
		float fill_w = track_w * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, track_x, track_y, fill_w, track_h, 2.0f);
		nvgFillColor(vg, nvgRGBA(0, 120, 215, 255));
		nvgFill(vg);

		// Thumb
		float thumbR = FromDIP(SLIDER_THUMB_RADIUS);
		nvgBeginPath(vg);
		nvgCircle(vg, track_x + fill_w, track_y + track_h * 0.5f, thumbR);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);

		// Thumb shadow
		nvgBeginPath(vg);
		nvgCircle(vg, track_x + fill_w, track_y + track_h * 0.5f, thumbR);
		nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 60));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Value
	std::string val_str = std::format("{}", value);
	nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
	nvgText(vg, x + w - 5, y + h * 0.5f, val_str.c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const wxString& label, bool value, bool hover) {
	float x = rect.x;
	float y = rect.y;
	float w = rect.width;
	float h = rect.height;

	float box_sz = FromDIP(14);
	float box_y = y + (h - box_sz) * 0.5f;
	float box_x = x;

	// Box
	nvgBeginPath(vg);
	nvgRoundedRect(vg, box_x, box_y, box_sz, box_sz, 3.0f);
	if (value) {
		nvgFillColor(vg, nvgRGBA(0, 120, 215, 255));
	} else {
		nvgFillColor(vg, nvgRGBA(45, 45, 50, 255));
	}
	nvgFill(vg);

	// Border
	nvgBeginPath(vg);
	nvgRoundedRect(vg, box_x + 0.5f, box_y + 0.5f, box_sz - 1, box_sz - 1, 3.0f);
	if (hover) {
		nvgStrokeColor(vg, nvgRGBA(0, 150, 255, 255));
	} else {
		nvgStrokeColor(vg, nvgRGBA(80, 80, 80, 255));
	}
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	// Checkmark
	if (value) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, box_x + 3, box_y + 7);
		nvgLineTo(vg, box_x + 6, box_y + 10);
		nvgLineTo(vg, box_x + box_sz - 3, box_y + 4);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, box_x + box_sz + FromDIP(8), box_y + box_sz * 0.5f, label.ToStdString().c_str(), nullptr);
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

	bool hand_cursor = false;

	// Tools
	for (auto& tr : tool_rects) {
		if (tr.rect.Contains(m_hoverPos)) {
			hover_brush = tr.brush;
			if (prev_hover != hover_brush) {
				SetToolTip(tr.tooltip);
			}
			hand_cursor = true;
			break;
		}
	}
	if (!hover_brush && prev_hover) {
		UnsetToolTip();
	}

	// Sliders Interaction
	if (evt.LeftDown()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_size = true;
			CaptureMouse();
			int val = CalculateSliderValue(interactables.size_slider_rect, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			interactables.dragging_thickness = true;
			CaptureMouse();
			int val = CalculateSliderValue(interactables.thickness_slider_rect, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS);
			if (val != current_thickness) {
				current_thickness = val;
				g_brush_manager.SetBrushThickness(true, current_thickness, 100);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
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
			g_gui.SetStatusText(std::format("Selected Tool: {}", wxstr(hover_brush->getName()).ToStdString()));
		}
	} else if (evt.LeftDClick()) {
		if (interactables.size_slider_rect.Contains(m_hoverPos)) {
			current_size = MIN_BRUSH_SIZE;
			g_brush_manager.SetBrushSize(current_size);
			Refresh();
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		} else if (interactables.thickness_slider_rect.Contains(m_hoverPos)) {
			current_thickness = MAX_BRUSH_THICKNESS;
			g_brush_manager.SetBrushThickness(true, current_thickness, 100);
			Refresh();
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
		}
	}

	if (evt.Dragging() && evt.LeftIsDown()) {
		if (interactables.dragging_size) {
			int val = CalculateSliderValue(interactables.size_slider_rect, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
			if (val != current_size) {
				current_size = val;
				g_brush_manager.SetBrushSize(current_size);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Brush Size: {}", current_size));
		}
		if (interactables.dragging_thickness) {
			int val = CalculateSliderValue(interactables.thickness_slider_rect, MIN_BRUSH_THICKNESS, MAX_BRUSH_THICKNESS);
			if (val != current_thickness) {
				current_thickness = val;
				g_brush_manager.SetBrushThickness(true, current_thickness, 100);
				Refresh();
			}
			g_gui.SetStatusText(std::format("Thickness: {}", current_thickness));
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
		hand_cursor = true;
	}
	if (interactables.lock_check_rect.Contains(m_hoverPos)) {
		interactables.hover_lock = true;
		hand_cursor = true;
	}

	if (interactables.size_slider_rect.Contains(m_hoverPos) || interactables.thickness_slider_rect.Contains(m_hoverPos)) {
		hand_cursor = true;
	}

	SetCursor(hand_cursor ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));

	if (prev_hover != hover_brush || prev_preview != interactables.hover_preview || prev_lock != interactables.hover_lock) {
		Refresh();
	}
}

void ToolOptionsSurface::OnLeave(wxMouseEvent& evt) {
	hover_brush = nullptr;
	Refresh();
}

void ToolOptionsSurface::OnTimer(wxTimerEvent& evt) {
	// Trigger repaint for animation
	Refresh();

	// Check if we can stop timer
	bool animating = false;
	for (const auto& tr : tool_rects) {
		if (tr.hoverAnim > 0.01f) {
			animating = true;
			break;
		}
	}
	if (!animating) {
		m_animTimer.Stop();
	}
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

int ToolOptionsSurface::CalculateSliderValue(const wxRect& sliderRect, int min, int max) const {
	int label_w = FromDIP(SLIDER_LABEL_WIDTH);
	int track_x = sliderRect.GetLeft() + label_w;
	int track_w = sliderRect.width - label_w - FromDIP(SLIDER_TEXT_MARGIN);

	int rel_x = m_hoverPos.x - track_x;
	float pct = std::clamp(static_cast<float>(rel_x) / static_cast<float>(track_w), 0.0f, 1.0f);

	return min + static_cast<int>(pct * (max - min));
}
