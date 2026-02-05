#include "ui/tool_options_surface.h"
#include "app/main.h"
#include "app/settings.h"
#include "ui/gui.h"
#include "rendering/core/graphics.h"
#include "ui/artprovider.h"
#include "brushes/managers/brush_manager.h"
#include "brushes/brush.h"
#include "game/sprites.h"

// Bring in specific brushes for identification/selection
#include "brushes/border/optional_border_brush.h"
#include "brushes/door/door_brush.h"
#include "brushes/flag/flag_brush.h"

#include <glad/glad.h>
#include <nanovg.h>
#include <nanovg_gl.h>
#include <format>

ToolOptionsSurface::ToolOptionsSurface(wxWindow* parent) :
	NanoVGCanvas(parent, wxID_ANY) {
	m_animTimer.SetOwner(this);

	// NanoVGCanvas handles Paint, Size internally for GL setup, but we need our logic too
	// Note: NanoVGCanvas already binds OnPaint, OnSize.
	// We override OnNanoVGPaint which is called by NanoVGCanvas::OnPaint.
	// However, we need to handle mouse interactions.

	Bind(wxEVT_LEFT_DOWN, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEFT_UP, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_MOTION, &ToolOptionsSurface::OnMouse, this);
	Bind(wxEVT_LEAVE_WINDOW, &ToolOptionsSurface::OnLeave, this);
	Bind(wxEVT_SIZE, &ToolOptionsSurface::OnSize, this); // We also need to rebuild layout
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
	wxGLCanvas::DoSetSizeHints(minW, minH, maxW, maxH, incW, incH);
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
	// Background - sleek dark grey
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, width, height);
	nvgFillColor(vg, nvgRGBA(50, 50, 50, 255));
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
	wxRect r = tr.rect;

	float x = r.x;
	float y = r.y;
	float w = r.width;
	float h = r.height;

	// Background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, w, h, 4.0f);

	if (is_selected) {
		nvgFillColor(vg, nvgRGBA(0, 120, 215, 255)); // Highlight blue
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	} else if (is_hover) {
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 30));
		nvgFill(vg);
	} else {
		// Subtle button bg
		nvgFillColor(vg, nvgRGBA(70, 70, 70, 255));
		nvgFill(vg);
	}

	// Brush Sprite
	if (tr.brush) {
		int img = GetOrCreateBrushImage(vg, tr.brush);
		if (img > 0) {
			int icon_sz = std::min(r.width, r.height) - 4;
			int ix = r.x + (r.width - icon_sz) / 2;
			int iy = r.y + (r.height - icon_sz) / 2;

			NVGpaint p = nvgImagePattern(vg, ix, iy, icon_sz, icon_sz, 0, img, 1.0f);
			nvgBeginPath(vg);
			nvgRoundedRect(vg, ix, iy, icon_sz, icon_sz, 2.0f);
			nvgFillPaint(vg, p);
			nvgFill(vg);
		} else {
			// Fallback text
			nvgFontSize(vg, 12.0f);
			nvgFontFace(vg, "sans");
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
			wxString label = tr.tooltip.Left(1);
			nvgText(vg, x + w / 2, y + h / 2, label.ToUTF8().data(), nullptr);
		}
	}
}

void ToolOptionsSurface::DrawSlider(NVGcontext* vg, const wxRect& rect, const wxString& label, int value, int min, int max, bool active) {
	float x = rect.x;
	float y = rect.y;
	float h = rect.height;

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, x, y + h / 2, label.ToUTF8().data(), nullptr);

	// Track
	int label_w = FromDIP(70);
	float track_x = x + label_w;
	float track_w = rect.width - label_w - FromDIP(40);
	float track_h = 4.0f;
	float track_y = y + (h - track_h) / 2;

	nvgBeginPath(vg);
	nvgRoundedRect(vg, track_x, track_y, track_w, track_h, 2.0f);
	nvgFillColor(vg, nvgRGBA(80, 80, 80, 255));
	nvgFill(vg);

	// Fill
	if (value >= min && max > min) {
		float pct = (float)(value - min) / (float)(max - min);
		float fill_w = track_w * pct;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, track_x, track_y, fill_w, track_h, 2.0f);
		nvgFillColor(vg, nvgRGBA(0, 120, 215, 255));
		nvgFill(vg);

		// Thumb
		nvgBeginPath(vg);
		nvgCircle(vg, track_x + fill_w, track_y + track_h / 2, 6.0f);
		nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
		nvgFill(vg);
		// Shadow
		nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 100));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Value Text
	std::string val_str = std::format("{}", value);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, track_x + track_w + 10, y + h / 2, val_str.c_str(), nullptr);
}

void ToolOptionsSurface::DrawCheckbox(NVGcontext* vg, const wxRect& rect, const wxString& label, bool value, bool hover) {
	float x = rect.x;
	float y = rect.y;
	float h = rect.height;

	float box_sz = 16.0f;
	float box_y = y + (h - box_sz) / 2;

	// Box
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, box_y, box_sz, box_sz, 3.0f);
	if (value) {
		nvgFillColor(vg, nvgRGBA(0, 120, 215, 255));
	} else {
		nvgFillColor(vg, nvgRGBA(80, 80, 80, 255));
	}
	nvgFill(vg);

	if (hover) {
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 100));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);
	}

	// Checkmark
	if (value) {
		nvgBeginPath(vg);
		nvgMoveTo(vg, x + 4, box_y + 8);
		nvgLineTo(vg, x + 7, box_y + 11);
		nvgLineTo(vg, x + 12, box_y + 5);
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgStrokeWidth(vg, 2.0f);
		nvgStroke(vg);
	}

	// Label
	nvgFontSize(vg, 12.0f);
	nvgFontFace(vg, "sans");
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgText(vg, x + box_sz + 8, y + h / 2, label.ToUTF8().data(), nullptr);
}

int ToolOptionsSurface::GetOrCreateBrushImage(NVGcontext* vg, Brush* brush) {
	if (!brush) {
		return 0;
	}

	// Use brush pointer as ID (safe within session)
	uint32_t brushId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(brush) & 0xFFFFFFFF);

	int existing = GetCachedImage(brushId);
	if (existing > 0) {
		return existing;
	}

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

	// Calculate composite size (GameSprite logic reused from VirtualBrushGrid)
	int w = gs->width * 32;
	int h = gs->height * 32;
	if (w <= 0 || h <= 0) {
		return 0;
	}

	std::vector<uint8_t> composite(w * h * 4, 0);

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

	return GetOrCreateImage(brushId, composite.data(), w, h);
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
				g_brush_manager.SetBrushSize(current_size);
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
	// Anim logic
}

void ToolOptionsSurface::OnSize(wxSizeEvent& evt) {
	RebuildLayout();
	// Base class handles GL resize
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
