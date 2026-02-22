#include "rendering/drawers/overlays/hud_drawer.h"
#include "rendering/ui/map_display.h"
#include "ui/gui.h"
#include "ui/theme.h"
#include "brushes/brush.h"
#include "game/sprites.h"
#include "editor/editor.h"
#include "rendering/core/text_renderer.h"
#include "util/nvg_utils.h"
#include <format>
#include <algorithm>

HUDDrawer::HUDDrawer() {
}

HUDDrawer::~HUDDrawer() {
	if (lastContext) {
		ClearCache(lastContext);
	}
}

void HUDDrawer::ClearCache(NVGcontext* vg) {
	for (auto& pair : spriteCache) {
		if (pair.second > 0) {
			nvgDeleteImage(vg, pair.second);
		}
	}
	spriteCache.clear();
}

int HUDDrawer::getSpriteImage(NVGcontext* vg, Sprite* sprite) {
	if (!sprite) {
		return 0;
	}

	if (vg != lastContext) {
		if (lastContext) {
			ClearCache(lastContext);
		}
		spriteCache.clear();
		lastContext = vg;
	}

	uint64_t spriteId = reinterpret_cast<uint64_t>(sprite);
	auto it = spriteCache.find(spriteId);
	if (it != spriteCache.end()) {
		return it->second;
	}

	// Create texture logic (adapted from TooltipDrawer/NanoVGCanvas)
	// Try to get as GameSprite for RGBA access (Fast Path)
	GameSprite* gs = dynamic_cast<GameSprite*>(sprite);
	if (gs) {
		int w, h;
		auto composite = NvgUtils::CreateCompositeRGBA(*gs, w, h);
		if (composite) {
			int image = nvgCreateImageRGBA(vg, w, h, 0, composite.get());
			if (image > 0) {
				spriteCache[spriteId] = image;
				return image;
			}
		}
	} else {
		// Fallback for non-GameSprite (e.g. EditorSprite) or if fast path failed?
		// NvgUtils::CreateCompositeRGBA only handles GameSprite.
		// If we encounter other sprite types here, we might need the slow path (wxDC based).
		// For now, let's assume brushes mainly use GameSprites or we accept missing icon.
		// Brush::getSprite() usually returns a GameSprite.
	}

	return 0;
}

void HUDDrawer::Draw(NVGcontext* vg, MapCanvas* canvas) {
	if (!vg || !canvas) {
		return;
	}

	nvgSave(vg);

	int mouseX = canvas->cursor_x;
	int mouseY = canvas->cursor_y;

	// Position: Offset from cursor
	float x = static_cast<float>(mouseX) + 20.0f;
	float y = static_cast<float>(mouseY) + 20.0f;

	// Clamp to screen bounds
	int screenW = canvas->GetSize().x;
	int screenH = canvas->GetSize().y;

	// Prepare content
	std::vector<std::string> lines;
	std::string title;
	int iconImage = 0;

	// 1. Coordinates
	Position pos = canvas->GetCursorPosition();
	lines.push_back(std::format("X: {}  Y: {}  Z: {}", pos.x, pos.y, pos.z));

	// 2. Mode Info
	if (g_gui.IsSelectionMode()) {
		if (!canvas->editor.selection.empty()) {
			title = "Selection";
			lines.push_back(std::format("Count: {}", canvas->editor.selection.size()));

			Position minPos = canvas->editor.selection.minPosition();
			Position maxPos = canvas->editor.selection.maxPosition();
			int w = maxPos.x - minPos.x + 1;
			int h = maxPos.y - minPos.y + 1;
			lines.push_back(std::format("Size: {} x {}", w, h));
		} else {
			title = "Selection Mode";
			lines.push_back("(Drag to select)");
		}
	} else {
		// Drawing Mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (brush) {
			title = brush->getName();
			Sprite* spr = brush->getSprite();
			if (!spr) {
				spr = g_gui.gfx.getSprite(brush->getLookID());
			}
			iconImage = getSpriteImage(vg, spr);
		} else {
			title = "No Brush";
		}
	}

	// Layout calculations
	float padding = 8.0f;
	float iconSize = 32.0f;
	float lineHeight = 16.0f;
	float fontSize = 12.0f;
	float titleFontSize = 13.0f;

	nvgFontSize(vg, fontSize);
	nvgFontFace(vg, "sans");

	float maxTextWidth = 0.0f;
	for (const auto& line : lines) {
		float bounds[4];
		nvgTextBounds(vg, 0, 0, line.c_str(), nullptr, bounds);
		maxTextWidth = std::max(maxTextWidth, bounds[2] - bounds[0]);
	}

	float titleWidth = 0.0f;
	if (!title.empty()) {
		nvgFontSize(vg, titleFontSize);
		float bounds[4];
		nvgTextBounds(vg, 0, 0, title.c_str(), nullptr, bounds);
		titleWidth = bounds[2] - bounds[0];
	}

	float contentWidth = std::max(maxTextWidth, titleWidth);
	if (iconImage > 0) {
		contentWidth += iconSize + padding;
	}

	float width = contentWidth + padding * 2;
	float height = padding * 2 + lines.size() * lineHeight;
	if (!title.empty()) {
		height += lineHeight + 4.0f; // Title height + gap
	}
	if (iconImage > 0) {
		height = std::max(height, iconSize + padding * 2);
	}

	// Adjust position to stay on screen
	if (x + width > screenW) {
		x = static_cast<float>(mouseX) - width - 10.0f;
	}
	if (y + height > screenH) {
		y = static_cast<float>(mouseY) - height - 10.0f;
	}

	// Draw Background (Glass effect)
	nvgBeginPath(vg);
	nvgRoundedRect(vg, x, y, width, height, 4.0f);
	nvgFillColor(vg, nvgRGBA(20, 20, 30, 200)); // Dark semi-transparent
	nvgFill(vg);

	// Border
	nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 100));
	nvgStrokeWidth(vg, 1.0f);
	nvgStroke(vg);

	// Draw Icon
	float textX = x + padding;
	float curY = y + padding;

	if (iconImage > 0) {
		nvgBeginPath(vg);
		nvgRect(vg, x + padding, y + padding, iconSize, iconSize);
		nvgFillPaint(vg, nvgImagePattern(vg, x + padding, y + padding, iconSize, iconSize, 0, iconImage, 1.0f));
		nvgFill(vg);
		textX += iconSize + padding;
	}

	// Draw Title
	if (!title.empty()) {
		nvgFontSize(vg, titleFontSize);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, textX, curY, title.c_str(), nullptr);
		curY += lineHeight + 4.0f;
	}

	// Draw Lines
	nvgFontSize(vg, fontSize);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
	for (const auto& line : lines) {
		nvgText(vg, textX, curY, line.c_str(), nullptr);
		curY += lineHeight;
	}

	nvgRestore(vg);
}
