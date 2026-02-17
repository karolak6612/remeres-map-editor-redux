//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/core/editor_sprite.h"

EditorSprite::EditorSprite(std::unique_ptr<wxBitmap> b16x16, std::unique_ptr<wxBitmap> b32x32) {
	bm[SPRITE_SIZE_16x16] = std::move(b16x16);
	bm[SPRITE_SIZE_32x32] = std::move(b32x32);
}

EditorSprite::~EditorSprite() {
	// Unique pointers clean themselves up, but unloadDC allows explicit clearing
	unloadDC();
}

void EditorSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	wxBitmap* sp = bm[sz].get();
	if (sp) {
		dc->DrawBitmap(*sp, start_x, start_y, true);
	}
}

void EditorSprite::unloadDC() {
	bm[SPRITE_SIZE_16x16].reset();
	bm[SPRITE_SIZE_32x32].reset();
}

std::unique_ptr<uint8_t[]> EditorSprite::GetRGBAData(int& width, int& height) {
	wxBitmap* sp = bm[SPRITE_SIZE_32x32].get();
	if (!sp || !sp->IsOk()) {
		width = 0;
		height = 0;
		return nullptr;
	}

	wxImage img = sp->ConvertToImage();
	if (!img.IsOk()) {
		width = 0;
		height = 0;
		return nullptr;
	}

	width = img.GetWidth();
	height = img.GetHeight();

	size_t size = width * height * 4;
	auto buffer = std::make_unique<uint8_t[]>(size);

	uint8_t* data = img.GetData();
	uint8_t* alpha = img.GetAlpha();
	bool hasAlpha = img.HasAlpha();

	for (int i = 0; i < width * height; ++i) {
		buffer[i*4 + 0] = data[i*3 + 0];
		buffer[i*4 + 1] = data[i*3 + 1];
		buffer[i*4 + 2] = data[i*3 + 2];
		if (hasAlpha && alpha) {
			buffer[i*4 + 3] = alpha[i];
		} else {
			buffer[i*4 + 3] = 255;
		}
	}

	return buffer;
}
