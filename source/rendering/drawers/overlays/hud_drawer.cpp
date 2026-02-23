#include "app/main.h"
#include "rendering/drawers/overlays/hud_drawer.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "brushes/brush.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "util/nvg_utils.h"
#include "rendering/ui/map_display.h"

#include <nanovg.h>
#include <format>

HUDDrawer::HUDDrawer() {
}

HUDDrawer::~HUDDrawer() {
}

void HUDDrawer::draw(NVGcontext* vg, const RenderView& view, Editor& editor, const DrawingOptions& options) {
	DrawCoords(vg, view, editor);
	DrawBrushInfo(vg, view, editor);

	if (!editor.selection.empty()) {
		DrawSelectionInfo(vg, view, editor, options);
	}
}

void HUDDrawer::DrawCoords(NVGcontext* vg, const RenderView& view, Editor& editor) {
	// Bottom right corner
	float w = static_cast<float>(view.screensize_x);
	float h = static_cast<float>(view.screensize_y);
	float padding = 10.0f;
	float pillHeight = 24.0f;

	// Get coords
	int mx = view.mouse_map_x;
	int my = view.mouse_map_y;
	int mz = view.floor;

	std::string text = std::format("X: {}  Y: {}  Z: {}", mx, my, mz);

	nvgFontSize(vg, 13.0f);
	nvgFontFace(vg, "sans-bold");
	float textWidth = nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, nullptr);
	float pillWidth = textWidth + 24.0f;

	float x = w - pillWidth - padding;
	float y = h - pillHeight - padding;

	// Shadow
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 2, y + 2, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 64));
	nvgFill(vg);

	// Pill background (Glassy)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(30, 30, 30, 200));
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 30));
	nvgStrokeWidth(vg, 1.0f);
	nvgFill(vg);
	nvgStroke(vg);

	// Text
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, x + 12.0f, y + pillHeight / 2.0f, text.c_str(), nullptr);
}

void HUDDrawer::DrawBrushInfo(NVGcontext* vg, const RenderView& view, Editor& editor) {
	Brush* brush = g_gui.GetCurrentBrush();
	if (!brush) return;

	float padding = 10.0f;
	float pillHeight = 28.0f;

	std::string text = brush->getName().ToStdString();

	nvgFontSize(vg, 14.0f);
	nvgFontFace(vg, "sans-bold");
	float textWidth = nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, nullptr);

	// Icon space?
	float iconSize = 20.0f;
	float iconPadding = 8.0f;
	float pillWidth = textWidth + 24.0f + iconSize + iconPadding;

	float x = padding;
	float y = padding; // Top left

	// Shadow
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 2, y + 2, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 64));
	nvgFill(vg);

	// Pill background
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(40, 44, 52, 220)); // Dark blue-ish grey

	// Accent border on left?
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 30));
	nvgStrokeWidth(vg, 1.0f);
	nvgFill(vg);
	nvgStroke(vg);

	// Draw Icon
	Sprite* spr = brush->getSprite();
	if (!spr) spr = g_gui.gfx.getSprite(brush->getLookID());

	if (spr) {
		// We need a way to get GL texture for sprite here.
		// MapCanvas has helpers but we don't have access to it easily.
		// However, global `g_gui` or similar might help, or we pass it.
		// `GetOrCreateSpriteTexture` is in `NanoVGCanvas` or `VirtualBrushGrid`.
		// We can duplicate the logic or skip icon for now.
		// Let's try to draw a simple colored circle or skip icon if too complex.
		// Actually, `MapCanvas` has `GetOrCreateSpriteTexture` but we don't have `MapCanvas`.
		// But we can use `GetOrCreateSpriteTexture` if we make it available or copy it.
		// For now, let's just draw the text.

		// Update pill width to remove icon space
		pillWidth = textWidth + 24.0f;

		// Redraw background with correct width
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, pillWidth, pillHeight, pillHeight / 2.0f);
		nvgFillColor(vg, nvgRGBA(40, 44, 52, 220));
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 30));
		nvgStrokeWidth(vg, 1.0f);
		nvgFill(vg);
		nvgStroke(vg);

		// Text
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, x + 12.0f, y + pillHeight / 2.0f, text.c_str(), nullptr);
	} else {
		// Just text
		pillWidth = textWidth + 24.0f;
		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, pillWidth, pillHeight, pillHeight / 2.0f);
		nvgFillColor(vg, nvgRGBA(40, 44, 52, 220));
		nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 30));
		nvgStrokeWidth(vg, 1.0f);
		nvgFill(vg);
		nvgStroke(vg);

		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(vg, x + 12.0f, y + pillHeight / 2.0f, text.c_str(), nullptr);
	}
}

void HUDDrawer::DrawSelectionInfo(NVGcontext* vg, const RenderView& view, Editor& editor, const DrawingOptions& options) {
	// Show count
	size_t count = editor.selection.size();
	std::string text = std::format("{} items selected", count);

	float padding = 10.0f;
	float pillHeight = 24.0f;

	nvgFontSize(vg, 13.0f);
	nvgFontFace(vg, "sans");
	float textWidth = nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, nullptr);
	float pillWidth = textWidth + 24.0f;

	float x = view.screensize_x / 2.0f - pillWidth / 2.0f; // Center bottom
	float y = view.screensize_y - pillHeight - padding;

	// Shadow
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x + 2, y + 2, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 64));
	nvgFill(vg);

	// Pill
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, pillWidth, pillHeight, pillHeight / 2.0f);
	nvgFillColor(vg, nvgRGBA(52, 152, 219, 220)); // Accent blue
	nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 50));
	nvgStrokeWidth(vg, 1.0f);
	nvgFill(vg);
	nvgStroke(vg);

	// Text
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgText(vg, x + 12.0f, y + pillHeight / 2.0f, text.c_str(), nullptr);
}
