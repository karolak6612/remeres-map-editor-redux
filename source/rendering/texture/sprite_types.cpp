//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "sprite_types.h"
#include "settings.h"
#include "gui.h"
#include <wx/memory.h>

// All 133 template colors
static const uint32_t TemplateOutfitLookupTable[] = {
	0xFFFFFF,
	0xFFD4BF,
	0xFFE9BF,
	0xFFFFBF,
	0xE9FFBF,
	0xD4FFBF,
	0xBFFFBF,
	0xBFFFD4,
	0xBFFFE9,
	0xBFFFFF,
	0xBFE9FF,
	0xBFD4FF,
	0xBFBFFF,
	0xD4BFFF,
	0xE9BFFF,
	0xFFBFFF,
	0xFFBFE9,
	0xFFBFD4,
	0xFFBFBF,
	0xDADADA,
	0xBF9F8F,
	0xBFAF8F,
	0xBFBF8F,
	0xAFBF8F,
	0x9FBF8F,
	0x8FBF8F,
	0x8FBF9F,
	0x8FBFAF,
	0x8FBFBF,
	0x8FAFBF,
	0x8F9FBF,
	0x8F8FBF,
	0x9F8FBF,
	0xAF8FBF,
	0xBF8FBF,
	0xBF8FAF,
	0xBF8F9F,
	0xBF8F8F,
	0xB6B6B6,
	0xBF7F5F,
	0xBFAF8F,
	0xBFBF5F,
	0x9FBF5F,
	0x7FBF5F,
	0x5FBF5F,
	0x5FBF7F,
	0x5FBF9F,
	0x5FBFBF,
	0x5F9FBF,
	0x5F7FBF,
	0x5F5FBF,
	0x7F5FBF,
	0x9F5FBF,
	0xBF5FBF,
	0xBF5F9F,
	0xBF5F7F,
	0xBF5F5F,
	0x919191,
	0xBF6A3F,
	0xBF943F,
	0xBFBF3F,
	0x94BF3F,
	0x6ABF3F,
	0x3FBF3F,
	0x3FBF6A,
	0x3FBF94,
	0x3FBFBF,
	0x3F94BF,
	0x3F6ABF,
	0x3F3FBF,
	0x6A3FBF,
	0x943FBF,
	0xBF3FBF,
	0xBF3F94,
	0xBF3F6A,
	0xBF3F3F,
	0x6D6D6D,
	0xFF5500,
	0xFFAA00,
	0xFFFF00,
	0xAAFF00,
	0x54FF00,
	0x00FF00,
	0x00FF54,
	0x00FFAA,
	0x00FFFF,
	0x00A9FF,
	0x0055FF,
	0x0000FF,
	0x5500FF,
	0xA900FF,
	0xFE00FF,
	0xFF00AA,
	0xFF0055,
	0xFF0000,
	0x484848,
	0xBF3F00,
	0xBF7F00,
	0xBFBF00,
	0x7FBF00,
	0x3FBF00,
	0x00BF00,
	0x00BF3F,
	0x00BF7F,
	0x00BFBF,
	0x007FBF,
	0x003FBF,
	0x0000BF,
	0x3F00BF,
	0x7F00BF,
	0xBF00BF,
	0xBF007F,
	0xBF003F,
	0xBF0000,
	0x242424,
	0x7F2A00,
	0x7F5500,
	0x7F7F00,
	0x557F00,
	0x2A7F00,
	0x007F00,
	0x007F2A,
	0x007F55,
	0x007F7F,
	0x00547F,
	0x002A7F,
	0x00007F,
	0x2A007F,
	0x54007F,
	0x7F007F,
	0x7F0055,
	0x7F002A,
	0x7F0000,
};

// ============================================================================
// EditorSprite

EditorSprite::EditorSprite(wxBitmap* b16x16, wxBitmap* b32x32) {
	bm[SPRITE_SIZE_16x16] = b16x16;
	bm[SPRITE_SIZE_32x32] = b32x32;
}

EditorSprite::~EditorSprite() {
	unloadDC();
}

void EditorSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	wxBitmap* sp = bm[sz];
	if (sp) {
		dc->DrawBitmap(*sp, start_x, start_y, true);
	}
}

void EditorSprite::unloadDC() {
	delete bm[SPRITE_SIZE_16x16];
	delete bm[SPRITE_SIZE_32x32];
	bm[SPRITE_SIZE_16x16] = nullptr;
	bm[SPRITE_SIZE_32x32] = nullptr;
}

// ============================================================================
// GameSprite

GameSprite::GameSprite() :
	id(0),
	height(0),
	width(0),
	layers(0),
	pattern_x(0),
	pattern_y(0),
	pattern_z(0),
	frames(0),
	numsprites(0),
	animator(nullptr),
	draw_height(0),
	drawoffset_x(0),
	drawoffset_y(0),
	minimap_color(0) {
	dc[SPRITE_SIZE_16x16] = nullptr;
	dc[SPRITE_SIZE_32x32] = nullptr;
}

GameSprite::~GameSprite() {
	unloadDC();
	for (std::list<TemplateImage*>::iterator iter = instanced_templates.begin(); iter != instanced_templates.end(); ++iter) {
		delete *iter;
	}

	delete animator;
}

void GameSprite::clean(int time) {
	for (std::list<TemplateImage*>::iterator iter = instanced_templates.begin();
		 iter != instanced_templates.end();
		 ++iter) {
		(*iter)->clean(time);
	}
}

void GameSprite::unloadDC() {
	delete dc[SPRITE_SIZE_16x16];
	delete dc[SPRITE_SIZE_32x32];
	dc[SPRITE_SIZE_16x16] = nullptr;
	dc[SPRITE_SIZE_32x32] = nullptr;
}

int GameSprite::getDrawHeight() const {
	return draw_height;
}

std::pair<int, int> GameSprite::getDrawOffset() const {
	return std::make_pair(drawoffset_x, drawoffset_y);
}

uint8_t GameSprite::getMiniMapColor() const {
	return minimap_color;
}

int GameSprite::getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const {
	return ((((((frame % this->frames) * this->pattern_z + pattern_z) * this->pattern_y + pattern_y) * this->pattern_x + pattern_x) * this->layers + layer) * this->height + height) * this->width + width;
}

GLuint GameSprite::getHardwareID(int _x, int _y, int _layer, int _count, int _pattern_x, int _pattern_y, int _pattern_z, int _frame) {
	uint32_t v;
	if (_count >= 0 && height <= 1 && width <= 1) {
		v = _count;
	} else {
		v = ((((((_frame)*pattern_y + _pattern_y) * pattern_x + _pattern_x) * layers + _layer) * height + _y) * width + _x);
	}
	if (v >= numsprites) {
		if (numsprites == 1) {
			v = 0;
		} else {
			v %= numsprites;
		}
	}
	return spriteList[v]->getHardwareID();
}

GameSprite::TemplateImage* GameSprite::getTemplateImage(int sprite_index, const Outfit& outfit) {
	if (instanced_templates.empty()) {
		TemplateImage* img = newd TemplateImage(this, sprite_index, outfit);
		instanced_templates.push_back(img);
		return img;
	}
	for (std::list<TemplateImage*>::iterator iter = instanced_templates.begin(); iter != instanced_templates.end(); ++iter) {
		TemplateImage* img = *iter;
		if (img->sprite_index == sprite_index) {
			uint32_t lookHash = img->lookHead << 24 | img->lookBody << 16 | img->lookLegs << 8 | img->lookFeet;
			if (outfit.getColorHash() == lookHash) {
				return img;
			}
		}
	}
	TemplateImage* img = newd TemplateImage(this, sprite_index, outfit);
	instanced_templates.push_back(img);
	return img;
}

GLuint GameSprite::getHardwareID(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame) {
	uint32_t v = getIndex(_x, _y, 0, _dir, _addon, _pattern_z, _frame);
	if (v >= numsprites) {
		if (numsprites == 1) {
			v = 0;
		} else {
			v %= numsprites;
		}
	}
	if (layers > 1) { // Template
		TemplateImage* img = getTemplateImage(v, _outfit);
		return img->getHardwareID();
	}
	return spriteList[v]->getHardwareID();
}

wxMemoryDC* GameSprite::getDC(SpriteSize size) {
	ASSERT(size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_32x32);

	if (!dc[size]) {
		ASSERT(width >= 1 && height >= 1);

		const int bgshade = g_settings.getInteger(Config::ICON_BACKGROUND);

		int image_size = std::max<int>(width, height) * SPRITE_PIXELS;
		wxImage image(image_size, image_size);
		image.Clear(bgshade);

		for (uint8_t l = 0; l < layers; l++) {
			for (uint8_t w = 0; w < width; w++) {
				for (uint8_t h = 0; h < height; h++) {
					const int i = getIndex(w, h, l, 0, 0, 0, 0);
					uint8_t* data = spriteList[i]->getRGBData();
					if (data) {
						wxImage img(SPRITE_PIXELS, SPRITE_PIXELS, data);
						img.SetMaskColour(0xFF, 0x00, 0xFF);
						image.Paste(img, (width - w - 1) * SPRITE_PIXELS, (height - h - 1) * SPRITE_PIXELS);
						img.Destroy();
					}
				}
			}
		}

		if (size == SPRITE_SIZE_16x16 || image.GetWidth() > SPRITE_PIXELS || image.GetHeight() > SPRITE_PIXELS) {
			int new_size = SPRITE_SIZE_16x16 ? 16 : 32;
			image.Rescale(new_size, new_size);
		}

		wxBitmap bmp(image);
		dc[size] = newd wxMemoryDC(bmp);
		g_gui.gfx.addSpriteToCleanup(this);
		image.Destroy();
	}
	return dc[size];
}

void GameSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	if (width == -1) {
		width = sz == SPRITE_SIZE_32x32 ? 32 : 16;
	}
	if (height == -1) {
		height = sz == SPRITE_SIZE_32x32 ? 32 : 16;
	}
	wxDC* sdc = getDC(sz);
	if (sdc) {
		dc->Blit(start_x, start_y, width, height, sdc, 0, 0, wxCOPY, true);
	} else {
		const wxBrush& b = dc->GetBrush();
		dc->SetBrush(*wxRED_BRUSH);
		dc->DrawRectangle(start_x, start_y, width, height);
		dc->SetBrush(b);
	}
}

// ============================================================================
// GameSprite::Image

GameSprite::Image::Image() :
	isGLLoaded(false),
	lastaccess(0) {
}

GameSprite::Image::~Image() {
	unloadGLTexture(0);
}

void GameSprite::Image::createGLTexture(GLuint whatid) {
	ASSERT(!isGLLoaded);

	uint8_t* rgba = getRGBAData();
	if (!rgba) {
		return;
	}

	isGLLoaded = true;
	g_gui.gfx.loaded_textures += 1;

	glBindTexture(GL_TEXTURE_2D, whatid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SPRITE_PIXELS, SPRITE_PIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

	delete[] rgba;
}

void GameSprite::Image::unloadGLTexture(GLuint whatid) {
	if (isGLLoaded) {
		isGLLoaded = false;
		g_gui.gfx.loaded_textures -= 1;
		glDeleteTextures(1, &whatid);
	}
}

void GameSprite::Image::visit() {
	lastaccess = time(nullptr);
}

void GameSprite::Image::clean(int time) {
	if (isGLLoaded && time - lastaccess > g_settings.getInteger(Config::TEXTURE_LONGEVITY)) {
		unloadGLTexture(0);
	}
}

// ============================================================================
// GameSprite::NormalImage

GameSprite::NormalImage::NormalImage() :
	id(0),
	size(0),
	dump(nullptr) {
}

GameSprite::NormalImage::~NormalImage() {
	delete[] dump;
}

void GameSprite::NormalImage::clean(int time) {
	Image::clean(time);
	if (time - lastaccess > 5 && !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
		delete[] dump;
		dump = nullptr;
	}
}

uint8_t* GameSprite::NormalImage::getRGBData() {
	if (!dump) {
		if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
			return nullptr;
		}

		if (!g_gui.gfx.loadSpriteDump(dump, size, id)) {
			return nullptr;
		}
	}

	const int pixels_data_size = SPRITE_PIXELS * SPRITE_PIXELS * 3;
	uint8_t* data = newd uint8_t[pixels_data_size];
	uint8_t bpp = g_gui.gfx.hasTransparency() ? 4 : 3;
	int write = 0;
	int read = 0;

	while (read < size && write < pixels_data_size) {
		int transparent = dump[read] | dump[read + 1] << 8;
		read += 2;
		for (int i = 0; i < transparent && write < pixels_data_size; i++) {
			data[write + 0] = 0xFF; // red
			data[write + 1] = 0x00; // green
			data[write + 2] = 0xFF; // blue
			write += 3;
		}

		int colored = dump[read] | dump[read + 1] << 8;
		read += 2;
		for (int i = 0; i < colored && write < pixels_data_size; i++) {
			data[write + 0] = dump[read + 0]; // red
			data[write + 1] = dump[read + 1]; // green
			data[write + 2] = dump[read + 2]; // blue
			write += 3;
			read += bpp;
		}
	}

	while (write < pixels_data_size) {
		data[write + 0] = 0xFF; // red
		data[write + 1] = 0x00; // green
		data[write + 2] = 0xFF; // blue
		write += 3;
	}
	return data;
}

uint8_t* GameSprite::NormalImage::getRGBAData() {
	if (!dump) {
		if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
			return nullptr;
		}

		if (!g_gui.gfx.loadSpriteDump(dump, size, id)) {
			return nullptr;
		}
	}

	const int pixels_data_size = SPRITE_PIXELS_SIZE * 4;
	uint8_t* data = newd uint8_t[pixels_data_size];
	bool use_alpha = g_gui.gfx.hasTransparency();
	uint8_t bpp = use_alpha ? 4 : 3;
	int write = 0;
	int read = 0;

	while (read < size && write < pixels_data_size) {
		int transparent = dump[read] | dump[read + 1] << 8;
		if (use_alpha && transparent >= SPRITE_PIXELS_SIZE) {
			break;
		}
		read += 2;
		for (int i = 0; i < transparent && write < pixels_data_size; i++) {
			data[write + 0] = 0x00; // red
			data[write + 1] = 0x00; // green
			data[write + 2] = 0x00; // blue
			data[write + 3] = 0x00; // alpha
			write += 4;
		}

		int colored = dump[read] | dump[read + 1] << 8;
		read += 2;
		for (int i = 0; i < colored && write < pixels_data_size; i++) {
			data[write + 0] = dump[read + 0]; // red
			data[write + 1] = dump[read + 1]; // green
			data[write + 2] = dump[read + 2]; // blue
			data[write + 3] = use_alpha ? dump[read + 3] : 0xFF; // alpha
			write += 4;
			read += bpp;
		}
	}

	while (write < pixels_data_size) {
		data[write + 0] = 0x00; // red
		data[write + 1] = 0x00; // green
		data[write + 2] = 0x00; // blue
		data[write + 3] = 0x00; // alpha
		write += 4;
	}
	return data;
}

GLuint GameSprite::NormalImage::getHardwareID() {
	if (!isGLLoaded) {
		createGLTexture(id);
	}
	visit();
	return id;
}

void GameSprite::NormalImage::createGLTexture(GLuint ignored) {
	Image::createGLTexture(id);
}

void GameSprite::NormalImage::unloadGLTexture(GLuint ignored) {
	Image::unloadGLTexture(id);
}

// ============================================================================
// GameSprite::TemplateImage

GameSprite::TemplateImage::TemplateImage(GameSprite* parent, int v, const Outfit& outfit) :
	gl_tid(0),
	parent(parent),
	sprite_index(v),
	lookHead(outfit.lookHead),
	lookBody(outfit.lookBody),
	lookLegs(outfit.lookLegs),
	lookFeet(outfit.lookFeet) {
}

GameSprite::TemplateImage::~TemplateImage() {
}

void GameSprite::TemplateImage::colorizePixel(uint8_t color, uint8_t& red, uint8_t& green, uint8_t& blue) {
	uint8_t ro = (TemplateOutfitLookupTable[color] & 0xFF0000) >> 16;
	uint8_t go = (TemplateOutfitLookupTable[color] & 0xFF00) >> 8;
	uint8_t bo = (TemplateOutfitLookupTable[color] & 0xFF);
	red = (uint8_t)(red * (ro / 255.f));
	green = (uint8_t)(green * (go / 255.f));
	blue = (uint8_t)(blue * (bo / 255.f));
}

uint8_t* GameSprite::TemplateImage::getRGBData() {
	uint8_t* rgbdata = parent->spriteList[sprite_index]->getRGBData();
	uint8_t* template_rgbdata = parent->spriteList[sprite_index + parent->height * parent->width]->getRGBData();

	if (!rgbdata) {
		delete[] template_rgbdata;
		return nullptr;
	}
	if (!template_rgbdata) {
		delete[] rgbdata;
		return nullptr;
	}

	size_t tableSize = sizeof(TemplateOutfitLookupTable) / sizeof(TemplateOutfitLookupTable[0]);
	if (lookHead >= tableSize) {
		lookHead = 0;
	}
	if (lookBody >= tableSize) {
		lookBody = 0;
	}
	if (lookLegs >= tableSize) {
		lookLegs = 0;
	}
	if (lookFeet >= tableSize) {
		lookFeet = 0;
	}

	for (int y = 0; y < SPRITE_PIXELS; ++y) {
		for (int x = 0; x < SPRITE_PIXELS; ++x) {
			uint8_t& red = rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 0];
			uint8_t& green = rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 1];
			uint8_t& blue = rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 2];

			uint8_t& tred = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 0];
			uint8_t& tgreen = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 1];
			uint8_t& tblue = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 2];

			if (tred && tgreen && !tblue) { // yellow => head
				colorizePixel(lookHead, red, green, blue);
			} else if (tred && !tgreen && !tblue) { // red => body
				colorizePixel(lookBody, red, green, blue);
			} else if (!tred && tgreen && !tblue) { // green => legs
				colorizePixel(lookLegs, red, green, blue);
			} else if (!tred && !tgreen && tblue) { // blue => feet
				colorizePixel(lookFeet, red, green, blue);
			}
		}
	}
	delete[] template_rgbdata;
	return rgbdata;
}

uint8_t* GameSprite::TemplateImage::getRGBAData() {
	uint8_t* rgbadata = parent->spriteList[sprite_index]->getRGBAData();
	uint8_t* template_rgbdata = parent->spriteList[sprite_index + parent->height * parent->width]->getRGBData();

	if (!rgbadata) {
		delete[] template_rgbdata;
		return nullptr;
	}
	if (!template_rgbdata) {
		delete[] rgbadata;
		return nullptr;
	}

	size_t tableSize = sizeof(TemplateOutfitLookupTable) / sizeof(TemplateOutfitLookupTable[0]);
	if (lookHead >= tableSize) {
		lookHead = 0;
	}
	if (lookBody >= tableSize) {
		lookBody = 0;
	}
	if (lookLegs >= tableSize) {
		lookLegs = 0;
	}
	if (lookFeet >= tableSize) {
		lookFeet = 0;
	}

	for (int y = 0; y < SPRITE_PIXELS; ++y) {
		for (int x = 0; x < SPRITE_PIXELS; ++x) {
			uint8_t& red = rgbadata[y * SPRITE_PIXELS * 4 + x * 4 + 0];
			uint8_t& green = rgbadata[y * SPRITE_PIXELS * 4 + x * 4 + 1];
			uint8_t& blue = rgbadata[y * SPRITE_PIXELS * 4 + x * 4 + 2];

			uint8_t& tred = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 0];
			uint8_t& tgreen = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 1];
			uint8_t& tblue = template_rgbdata[y * SPRITE_PIXELS * 3 + x * 3 + 2];

			if (tred && tgreen && !tblue) { // yellow => head
				colorizePixel(lookHead, red, green, blue);
			} else if (tred && !tgreen && !tblue) { // red => body
				colorizePixel(lookBody, red, green, blue);
			} else if (!tred && tgreen && !tblue) { // green => legs
				colorizePixel(lookLegs, red, green, blue);
			} else if (!tred && !tgreen && tblue) { // blue => feet
				colorizePixel(lookFeet, red, green, blue);
			}
		}
	}
	delete[] template_rgbdata;
	return rgbadata;
}

GLuint GameSprite::TemplateImage::getHardwareID() {
	if (!isGLLoaded) {
		if (gl_tid == 0) {
			gl_tid = g_gui.gfx.getFreeTextureID();
		}
		createGLTexture(gl_tid);
		if (!isGLLoaded) {
			return 0;
		}
	}
	visit();
	return gl_tid;
}

void GameSprite::TemplateImage::createGLTexture(GLuint unused) {
	Image::createGLTexture(gl_tid);
}

void GameSprite::TemplateImage::unloadGLTexture(GLuint unused) {
	Image::unloadGLTexture(gl_tid);
}
