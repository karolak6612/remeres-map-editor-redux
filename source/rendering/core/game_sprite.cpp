//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/core/game_sprite.h"
#include "rendering/core/graphics.h"
#include "ui/gui.h"
#include "app/settings.h"
#include "rendering/utilities/sprite_icon_generator.h"
#include "rendering/core/outfit_colorizer.h"
#include "rendering/core/outfit_colors.h"
#include <spdlog/spdlog.h>
#include <atomic>
#include <algorithm>
#include <ranges>
#include <span>

static std::atomic<uint32_t> template_id_generator(0x1000000);

constexpr int RGB_COMPONENTS = 3;
constexpr int RGBA_COMPONENTS = 4;

CreatureSprite::CreatureSprite(GameSprite* parent, const Outfit& outfit) :
	parent(parent),
	outfit(outfit) {
}

CreatureSprite::~CreatureSprite() {
}

void CreatureSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	if (parent) {
		parent->DrawTo(dc, sz, outfit, start_x, start_y, width, height);
	}
}

void CreatureSprite::unloadDC() {
	if (parent) {
		GameSprite::RenderKey key;
		key.colorHash = outfit.getColorHash();
		key.mountColorHash = outfit.getMountColorHash();
		key.lookMount = outfit.lookMount;
		key.lookAddon = outfit.lookAddon;
		key.lookMountHead = outfit.lookMountHead;
		key.lookMountBody = outfit.lookMountBody;
		key.lookMountLegs = outfit.lookMountLegs;
		key.lookMountFeet = outfit.lookMountFeet;

		key.size = SPRITE_SIZE_16x16;
		parent->colored_dc.erase(key);

		key.size = SPRITE_SIZE_32x32;
		parent->colored_dc.erase(key);
	}
}

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
	minimap_color(0),
	is_simple(false) {
	// dc initialized to nullptr by unique_ptr default ctor
}

GameSprite::~GameSprite() {
	unloadDC();
	// instanced_templates and animator cleaned up automatically by unique_ptr
}

void GameSprite::ColorizeTemplatePixels(std::span<uint8_t> dest, std::span<const uint8_t> mask, const Outfit& outfit, bool destHasAlpha) {
	const int dest_step = destHasAlpha ? RGBA_COMPONENTS : RGB_COMPONENTS;
	const int mask_step = RGB_COMPONENTS;

	size_t pixelCount = mask.size() / mask_step;
	if (pixelCount * dest_step > dest.size()) {
		pixelCount = dest.size() / dest_step;
	}

	for (size_t i = 0; i < pixelCount; ++i) {
		uint8_t& red = dest[i * dest_step + 0];
		uint8_t& green = dest[i * dest_step + 1];
		uint8_t& blue = dest[i * dest_step + 2];

		const uint8_t& tred = mask[i * mask_step + 0];
		const uint8_t& tgreen = mask[i * mask_step + 1];
		const uint8_t& tblue = mask[i * mask_step + 2];

		if (tred && tgreen && !tblue) { // yellow => head
			OutfitColorizer::ColorizePixel(outfit.lookHead, red, green, blue);
		} else if (tred && !tgreen && !tblue) { // red => body
			OutfitColorizer::ColorizePixel(outfit.lookBody, red, green, blue);
		} else if (!tred && tgreen && !tblue) { // green => legs
			OutfitColorizer::ColorizePixel(outfit.lookLegs, red, green, blue);
		} else if (!tred && !tgreen && tblue) { // blue => feet
			OutfitColorizer::ColorizePixel(outfit.lookFeet, red, green, blue);
		}
	}
}

void GameSprite::clean(time_t time, int longevity) {
	for (auto& iter : instanced_templates) {
		iter->clean(time, longevity);
	}
}

void GameSprite::unloadDC() {
	dc[SPRITE_SIZE_16x16].reset();
	dc[SPRITE_SIZE_32x32].reset();
	dc[SPRITE_SIZE_64x64].reset();
	bm[SPRITE_SIZE_16x16].reset();
	bm[SPRITE_SIZE_32x32].reset();
	bm[SPRITE_SIZE_64x64].reset();
	colored_dc.clear();
}

int GameSprite::getDrawHeight() const {
	return draw_height;
}

bool GameSprite::isSimpleAndLoaded() const {
	return is_simple && spriteList[0]->isGLLoaded;
}

uint32_t GameSprite::getDebugImageId(size_t index) const {
	if (index < spriteList.size() && spriteList[index]->isNormalImage()) {
		return static_cast<const NormalImage*>(spriteList[index])->id;
	}
	return 0;
}

uint32_t GameSprite::getSpriteId(int frameIndex, int pattern_x, int pattern_y) const {
	auto idx = getIndex(width, height, 0, pattern_x, pattern_y, 0, frameIndex); // Assuming layer, pattern_z are 0 for this context
	if (idx >= 0 && static_cast<size_t>(idx) < spriteList.size() && spriteList[idx]->isNormalImage()) {
		return static_cast<const NormalImage*>(spriteList[idx])->id;
	}
	return 0;
}

std::pair<int, int> GameSprite::getDrawOffset() const {
	return std::make_pair(drawoffset_x, drawoffset_y);
}

uint8_t GameSprite::getMiniMapColor() const {
	return minimap_color;
}

size_t GameSprite::getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const {
	if (is_simple) {
		return 0;
	}
	if (this->frames == 0) {
		return 0;
	}
	size_t idx = (this->frames > 1) ? frame % this->frames : 0;
	// Cast operands to size_t to force 64-bit arithmetic and avoid overflow
	idx = idx * static_cast<size_t>(this->pattern_z) + static_cast<size_t>(pattern_z);
	idx = idx * static_cast<size_t>(this->pattern_y) + static_cast<size_t>(pattern_y);
	idx = idx * static_cast<size_t>(this->pattern_x) + static_cast<size_t>(pattern_x);
	idx = idx * static_cast<size_t>(this->layers) + static_cast<size_t>(layer);
	idx = idx * static_cast<size_t>(this->height) + static_cast<size_t>(height);
	idx = idx * static_cast<size_t>(this->width) + static_cast<size_t>(width);
	return idx;
}

const AtlasRegion* GameSprite::getAtlasRegion(const SpriteAtlasRequest& req) {
	if (numsprites == 0) {
		return nullptr;
	}

	// Optimization for simple static sprites (1x1, 1 frame, etc.)
	// Most ground tiles fall into this category.
	if (req.subtype == -1 && numsprites == 1 && frames == 1 && layers == 1 && width == 1 && height == 1) {
		// Also check default params
		if (req.x == 0 && req.y == 0 && req.layer == 0 && req.frame == 0 && req.pattern_x == 0 && req.pattern_y == 0 && req.pattern_z == 0) {
			// Check cache
			// We rely on spriteList[0] being valid for simple sprites
			// shared Sprite Fix: Verify generation ID matches what we cached.
			// Wrong Sprite Fix: Verify sprite ID matches what we cached.
			// Optimization: Check lightweight fields BEFORE calling heavy getAtlasRegion()
			if (cached_default_region && spriteList[0]->isGLLoaded && cached_generation_id == spriteList[0]->generation_id && cached_sprite_id == spriteList[0]->id) {
				return cached_default_region;
			}

			// Cache miss or staleness suspected: Use the getter to ensure self-healing check runs
			const AtlasRegion* valid_region = spriteList[0]->getAtlasRegion();
			if (valid_region && spriteList[0]->isGLLoaded) {
				cached_default_region = valid_region;
				cached_generation_id = spriteList[0]->generation_id;
				cached_sprite_id = spriteList[0]->id;
			} else {
				cached_default_region = nullptr;
				cached_generation_id = 0;
				cached_sprite_id = 0;
			}

			// Lazy set parent for cache invalidation (legacy path, kept for safety)
			spriteList[0]->parent = this;
			return valid_region;
		}
	}

	uint32_t v;
	if (req.subtype >= 0 && height <= 1 && width <= 1) {
		v = req.subtype;
	} else {
		v = ((((((req.frame) * pattern_y + req.pattern_y) * pattern_x + req.pattern_x) * layers + req.layer) * height + req.y) * width + req.x);
	}
	if (v >= numsprites) {
		if (numsprites == 1) {
			v = 0;
		} else {
			v %= numsprites;
		}
	}

	// Ensure parent is set for invalidation (even in slow path)
	if (spriteList[v]) {
		spriteList[v]->parent = this;
		return spriteList[v]->getAtlasRegion();
	}
	return nullptr;
}

GameSprite::TemplateImage* GameSprite::getTemplateImage(int sprite_index, const Outfit& outfit) {
	// While this is linear lookup, it is very rare for the list to contain more than 4-8 entries,
	// so it's faster than a hashmap anyways.
	auto it = std::ranges::find_if(instanced_templates, [sprite_index, &outfit](const auto& img) {
		if (img->sprite_index != sprite_index) {
			return false;
		}
		uint32_t lookHash = img->lookHead << 24 | img->lookBody << 16 | img->lookLegs << 8 | img->lookFeet;
		return outfit.getColorHash() == lookHash;
	});

	if (it != instanced_templates.end()) {
		// Move-to-front optimization
		if (it != instanced_templates.begin()) {
			std::iter_swap(it, instanced_templates.begin());
			return instanced_templates.front().get();
		}
		return it->get();
	}

	auto img = std::make_unique<TemplateImage>(this, sprite_index, outfit);
	TemplateImage* ptr = img.get();
	instanced_templates.push_back(std::move(img));
	return ptr;
}

const AtlasRegion* GameSprite::getAtlasRegion(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame) {
	if (numsprites == 0) {
		return nullptr;
	}

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
		return img->getAtlasRegion();
	}
	if (spriteList[v]) {
		spriteList[v]->parent = this;
		return spriteList[v]->getAtlasRegion();
	}
	return nullptr;
}

wxMemoryDC* GameSprite::getDC(SpriteSize size) {
	ASSERT(size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_32x32);

	if (!dc[size]) {
		wxBitmap bmp = SpriteIconGenerator::Generate(this, size);
		if (bmp.IsOk()) {
			bm[size] = std::make_unique<wxBitmap>(bmp);
			dc[size] = std::make_unique<wxMemoryDC>(*bm[size]);
		}
		g_gui.gfx.addSpriteToCleanup(this);
	}
	return dc[size].get();
}

wxMemoryDC* GameSprite::getDC(SpriteSize size, const Outfit& outfit) {
	ASSERT(size == SPRITE_SIZE_16x16 || size == SPRITE_SIZE_32x32);

	RenderKey key;
	key.size = size;
	key.colorHash = outfit.getColorHash();
	key.mountColorHash = outfit.getMountColorHash();
	key.lookMount = outfit.lookMount;
	key.lookAddon = outfit.lookAddon;
	key.lookMountHead = outfit.lookMountHead;
	key.lookMountBody = outfit.lookMountBody;
	key.lookMountLegs = outfit.lookMountLegs;
	key.lookMountFeet = outfit.lookMountFeet;

	auto it = colored_dc.find(key);
	if (it == colored_dc.end()) {
		wxBitmap bmp = SpriteIconGenerator::Generate(this, size, outfit);
		if (bmp.IsOk()) {
			auto cache = std::make_unique<CachedDC>();
			cache->bm = std::make_unique<wxBitmap>(bmp);
			cache->dc = std::make_unique<wxMemoryDC>(*cache->bm);

			auto res = colored_dc.insert(std::make_pair(key, std::move(cache)));
			g_gui.gfx.addSpriteToCleanup(this);
			return res.first->second->dc.get();
		}
		return nullptr;
	}
	return it->second->dc.get();
}

void GameSprite::DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width, int height) {
	const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
	int src_width = sprite_dim;
	int src_height = sprite_dim;

	if (width == -1) {
		width = src_width;
	}
	if (height == -1) {
		height = src_height;
	}
	wxDC* sdc = getDC(sz);
	if (sdc) {
		dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
	} else {
		const wxBrush& b = dc->GetBrush();
		dc->SetBrush(*wxRED_BRUSH);
		dc->DrawRectangle(start_x, start_y, width, height);
		dc->SetBrush(b);
	}
}

void GameSprite::DrawTo(wxDC* dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y, int width, int height) {
	const int sprite_dim = (sz == SPRITE_SIZE_64x64) ? 64 : (sz == SPRITE_SIZE_32x32 ? 32 : 16);
	int src_width = sprite_dim;
	int src_height = sprite_dim;

	if (width == -1) {
		width = src_width;
	}
	if (height == -1) {
		height = src_height;
	}
	wxDC* sdc = getDC(sz, outfit);
	if (sdc) {
		dc->StretchBlit(start_x, start_y, width, height, sdc, 0, 0, src_width, src_height, wxCOPY, true);
	} else {
		const wxBrush& b = dc->GetBrush();
		dc->SetBrush(*wxRED_BRUSH);
		dc->DrawRectangle(start_x, start_y, width, height);
		dc->SetBrush(b);
	}
}

GameSprite::Image::Image() :
	isGLLoaded(false),
	lastaccess(0) {
}

void GameSprite::Image::visit() const {
	lastaccess = static_cast<int64_t>(g_gui.gfx.getCachedTime());
}

void GameSprite::Image::clean(time_t time, int longevity) {
	// Base implementation does nothing
}

const AtlasRegion* GameSprite::Image::EnsureAtlasSprite(uint32_t sprite_id, std::unique_ptr<uint8_t[]> preloaded_data) {
	if (g_gui.gfx.ensureAtlasManager()) {
		AtlasManager* atlas_mgr = g_gui.gfx.getAtlasManager();

		// 1. Check if already loaded
		const AtlasRegion* region = atlas_mgr->getRegion(sprite_id);
		if (region) {
			// CRITICAL FIX: Check if the region we found is marked INVALID (from double-allocation fix)
			// or belongs to another sprite (mismatch).
			if (region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (region->debug_sprite_id != 0 && region->debug_sprite_id != sprite_id)) {
				spdlog::warn("STALE/INVALID MAP ENTRY DETECTED: Sprite {} maps to region owned by {}. Clearing mapping.", sprite_id, region->debug_sprite_id);
				// SAFETY: Only call clearMapping to avoid freeing shared slots owned by others.
				// removeSprite() is only for explicit destruction.
				atlas_mgr->clearMapping(sprite_id);
				region = nullptr; // Force reload
			} else {
				return region;
			}
		}

		// 2. Load data
		std::unique_ptr<uint8_t[]> rgba;
		if (preloaded_data) {
			rgba = std::move(preloaded_data);
		} else {
			rgba = getRGBAData();
		}

		if (!rgba) {
			// Fallback: Create a magenta texture to distinguish failure from garbage
			// Use literal 32 to ensure compilation (OT sprites are always 32x32)
			rgba = std::make_unique<uint8_t[]>(32 * 32 * RGBA_COMPONENTS);
			for (int i = 0; i < 32 * 32; ++i) {
				rgba[i * RGBA_COMPONENTS + 0] = 255;
				rgba[i * RGBA_COMPONENTS + 1] = 0;
				rgba[i * RGBA_COMPONENTS + 2] = 255;
				rgba[i * RGBA_COMPONENTS + 3] = 255;
			}
			spdlog::warn("getRGBAData returned null for sprite_id={} - using fallback", sprite_id);
		}

		// 3. Add to Atlas
		if (rgba) {
			region = atlas_mgr->addSprite(sprite_id, rgba.get());

			if (region) {
				isGLLoaded = true;
				g_gui.gfx.resident_images.push_back(this); // Add to resident set
				g_gui.gfx.collector.NotifyTextureLoaded();
				return region;
			} else {
				spdlog::warn("Atlas addSprite failed for sprite_id={}", sprite_id);
			}
		}
	} else {
		spdlog::error("AtlasManager not available for sprite_id={}", sprite_id);
	}
	return nullptr;
}

GameSprite::NormalImage::NormalImage() :
	id(0),
	atlas_region(nullptr),
	size(0),
	dump(nullptr) {
}

GameSprite::NormalImage::~NormalImage() {
	// dump auto-deleted
	if (isGLLoaded) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(id);
		}
	}
}

void GameSprite::NormalImage::fulfillPreload(std::unique_ptr<uint8_t[]> data) {
	atlas_region = EnsureAtlasSprite(id, std::move(data));
}

void GameSprite::NormalImage::clean(time_t time, int longevity) {
	// Evict from atlas if expired
	if (longevity == -1) {
		longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
	}
	if (isGLLoaded && time - static_cast<time_t>(lastaccess.load()) > longevity) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(id);
		}
		if (parent && parent->cached_default_region == atlas_region) {
			parent->cached_default_region = nullptr;
			parent->cached_generation_id = 0;
			parent->cached_sprite_id = 0;
		}

		isGLLoaded = false;
		atlas_region = nullptr;

		// Invalidate any pending preloads for this sprite ID
		generation_id++;

		g_gui.gfx.collector.NotifyTextureUnloaded();
	}

	if (time - static_cast<time_t>(lastaccess.load()) > 5 && !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) { // We keep dumps around for 5 seconds.
		dump.reset();
	}
}

std::unique_ptr<uint8_t[]> GameSprite::NormalImage::getRGBData() {
	if (id == 0) {
		const int pixels_data_size = SPRITE_PIXELS * SPRITE_PIXELS * 3;
		return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
	}

	if (!dump) {
		if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
			return nullptr;
		}

		if (!g_gui.gfx.loadSpriteDump(dump, size, id)) {
			return nullptr;
		}
	}

	return GameSprite::Decompress(std::span { dump.get(), size }, 3, true, g_gui.gfx.hasTransparency(), id);
}

namespace {

	struct DecompressionContext {
		int id;
		uint8_t bpp;
		bool source_has_alpha;
		bool& non_zero_alpha_found;
		bool& non_black_pixel_found;
		int output_channels;
		bool use_magic_pink;
	};

	bool ProcessTransparencyRun(std::span<const uint8_t> dump, size_t& read, std::span<uint8_t> data, size_t& write, DecompressionContext ctx) {
		if (read + 1 >= dump.size()) {
			return false;
		}
		int transparent = dump[read] | dump[read + 1] << 8;

		// Integrity check for transparency run
		if (write + (transparent * ctx.output_channels) > data.size()) {
			spdlog::warn("Sprite {}: Transparency run overrun (transparent={}, write={}, max={})", ctx.id, transparent, write, data.size());
			transparent = (data.size() - write) / ctx.output_channels;
		}

		read += 2;
		for (int i = 0; i < transparent; i++) {
			if (ctx.use_magic_pink) {
				data[write + 0] = 0xFF; // red
				data[write + 1] = 0x00; // green
				data[write + 2] = 0xFF; // blue
			} else {
				data[write + 0] = 0x00; // red
				data[write + 1] = 0x00; // green
				data[write + 2] = 0x00; // blue
				if (ctx.output_channels > 3) {
					data[write + 3] = 0x00; // alpha
				}
			}
			write += ctx.output_channels;
		}
		return true;
	}

	bool ProcessColoredRun(std::span<const uint8_t> dump, size_t& read, std::span<uint8_t> data, size_t& write, DecompressionContext ctx) {
		if (read + 1 >= dump.size()) {
			return false;
		}
		int colored = dump[read] | dump[read + 1] << 8;
		read += 2;

		// Integrity check for colored run
		if (write + (colored * ctx.output_channels) > data.size()) {
			spdlog::warn("Sprite {}: Colored run overrun (colored={}, write={}, max={})", ctx.id, colored, write, data.size());
			colored = (data.size() - write) / ctx.output_channels;
		}

		// Integrity check for read buffer
		if (read + (colored * ctx.bpp) > dump.size()) {
			spdlog::warn("Sprite {}: Read buffer overrun (colored={}, bpp={}, read={}, size={})", ctx.id, colored, ctx.bpp, read, dump.size());
			// We can't easily recover here without risking reading garbage, so stop
			return false;
		}

		for (int i = 0; i < colored; i++) {
			uint8_t r = dump[read + 0];
			uint8_t g = dump[read + 1];
			uint8_t b = dump[read + 2];
			uint8_t a = ctx.source_has_alpha ? dump[read + 3] : 0xFF;

			data[write + 0] = r;
			data[write + 1] = g;
			data[write + 2] = b;

			if (ctx.output_channels > 3) {
				data[write + 3] = a;
			}

			if (a > 0) {
				ctx.non_zero_alpha_found = true;
			}
			if (r > 0 || g > 0 || b > 0) {
				ctx.non_black_pixel_found = true;
			}

			write += ctx.output_channels;
			read += ctx.bpp;
		}
		return true;
	}

} // namespace

std::unique_ptr<uint8_t[]> GameSprite::Decompress(std::span<const uint8_t> dump, int output_channels, bool use_magic_pink, bool source_has_alpha, int id) {
	const int pixels_data_size = SPRITE_PIXELS_SIZE * output_channels;
	auto data_buffer = std::make_unique<uint8_t[]>(pixels_data_size);

	std::span<uint8_t> data(data_buffer.get(), pixels_data_size);

	uint8_t bpp = source_has_alpha ? 4 : 3;
	size_t write = 0;
	size_t read = 0;
	bool non_zero_alpha_found = false;
	bool non_black_pixel_found = false;

	DecompressionContext ctx {
		.id = id,
		.bpp = bpp,
		.source_has_alpha = source_has_alpha,
		.non_zero_alpha_found = non_zero_alpha_found,
		.non_black_pixel_found = non_black_pixel_found,
		.output_channels = output_channels,
		.use_magic_pink = use_magic_pink
	};

	// decompress pixels
	while (read < dump.size() && write < data.size()) {
		if (!ProcessTransparencyRun(dump, read, data, write, ctx)) {
			break;
		}

		if (read >= dump.size() || write >= data.size()) {
			break;
		}

		if (!ProcessColoredRun(dump, read, data, write, ctx)) {
			break;
		}
	}

	// fill remaining pixels
	while (write < data.size()) {
		if (use_magic_pink) {
			data[write + 0] = 0xFF; // red
			data[write + 1] = 0x00; // green
			data[write + 2] = 0xFF; // blue
		} else {
			data[write + 0] = 0x00; // red
			data[write + 1] = 0x00; // green
			data[write + 2] = 0x00; // blue
			if (output_channels > 3) {
				data[write + 3] = 0x00; // alpha
			}
		}
		write += output_channels;
	}

	// Debug logging for diagnostic - verify if we are decoding pure transparency or pure blackness
	if (!non_zero_alpha_found && id > 100) {
		static int empty_log_count = 0;
		if (empty_log_count++ < 10) {
			spdlog::info("Sprite {}: Decoded fully transparent sprite. bpp used: {}, dump size: {}", id, bpp, dump.size());
		}
	} else if (!non_black_pixel_found && non_zero_alpha_found && id > 100) {
		static int black_log_count = 0;
		if (black_log_count++ < 10) {
			spdlog::warn("Sprite {}: Decoded PURE BLACK sprite (Alpha > 0, RGB = 0). bpp used: {}, dump size: {}. Check hasTransparency() config!", id, bpp, dump.size());
		}
	}

	return data_buffer;
}

std::unique_ptr<uint8_t[]> GameSprite::NormalImage::getRGBAData() {
	// Robust ID 0 handling
	if (id == 0) {
		const int pixels_data_size = SPRITE_PIXELS_SIZE * 4;
		return std::make_unique<uint8_t[]>(pixels_data_size); // Value-initialized (zeroed)
	}

	if (!dump) {
		if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
			return nullptr;
		}

		if (!g_gui.gfx.loadSpriteDump(dump, size, id)) {
			// This is the only case where we return nullptr for non-zero ID
			// effectively warning the caller that the sprite is missing from file
			return nullptr;
		}
	}

	return GameSprite::Decompress(std::span { dump.get(), size }, 4, false, g_gui.gfx.hasTransparency(), id);
}

const AtlasRegion* GameSprite::NormalImage::getAtlasRegion() {
	if (isGLLoaded && atlas_region) {
		// Self-Healing: Check for stale atlas region pointer (e.g. from memory reuse)
		// Force reload if Owner is INVALID or DOES NOT MATCH
		if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != id)) {
			spdlog::warn("STALE ATLAS REGION DETECTED: NormalImage {} held region owned by {}. Force reloading.", id, atlas_region->debug_sprite_id);
			isGLLoaded = false;
			atlas_region = nullptr;
		} else {
			visit();
			return atlas_region;
		}
	}

	if (!isGLLoaded) {
		atlas_region = EnsureAtlasSprite(id);
	}
	visit();
	return atlas_region;
}

GameSprite::TemplateImage::TemplateImage(GameSprite* parent, int v, const Outfit& outfit) :
	atlas_region(nullptr),
	texture_id(template_id_generator.fetch_add(1)), // Generate unique ID for Atlas
	parent(parent),
	sprite_index(v),
	lookHead(outfit.lookHead),
	lookBody(outfit.lookBody),
	lookLegs(outfit.lookLegs),
	lookFeet(outfit.lookFeet) {
}

GameSprite::TemplateImage::~TemplateImage() {
	if (isGLLoaded) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(texture_id);
		}
	}
}

void GameSprite::TemplateImage::clean(time_t time, int longevity) {
	// Evict from atlas if expired
	if (longevity == -1) {
		longevity = g_settings.getInteger(Config::TEXTURE_LONGEVITY);
	}
	if (isGLLoaded && time - static_cast<time_t>(lastaccess.load()) > longevity) {
		if (g_gui.gfx.hasAtlasManager()) {
			g_gui.gfx.getAtlasManager()->removeSprite(texture_id);
		}
		isGLLoaded = false;
		atlas_region = nullptr;
		generation_id++;
		g_gui.gfx.collector.NotifyTextureUnloaded();
	}
}

std::unique_ptr<uint8_t[]> GameSprite::TemplateImage::getRGBData() {
	auto rgbdata = parent->spriteList[sprite_index]->getRGBData();
	auto template_rgbdata = parent->spriteList[sprite_index + parent->height * parent->width]->getRGBData();

	if (!rgbdata) {
		// template_rgbdata auto-deleted
		return nullptr;
	}
	if (!template_rgbdata) {
		// rgbdata auto-deleted
		return nullptr;
	}

	if (lookHead > TemplateOutfitLookupTableSize) {
		lookHead = 0;
	}
	if (lookBody > TemplateOutfitLookupTableSize) {
		lookBody = 0;
	}
	if (lookLegs > TemplateOutfitLookupTableSize) {
		lookLegs = 0;
	}
	if (lookFeet > TemplateOutfitLookupTableSize) {
		lookFeet = 0;
	}

	Outfit outfit;
	outfit.lookHead = lookHead;
	outfit.lookBody = lookBody;
	outfit.lookLegs = lookLegs;
	outfit.lookFeet = lookFeet;

	const size_t pixel_count = SPRITE_PIXELS * SPRITE_PIXELS;
	const size_t rgb_size = pixel_count * RGB_COMPONENTS;

	GameSprite::ColorizeTemplatePixels(std::span<uint8_t>(rgbdata.get(), rgb_size), std::span<uint8_t>(template_rgbdata.get(), rgb_size), outfit, false);

	// template_rgbdata auto-deleted
	return rgbdata;
}

std::unique_ptr<uint8_t[]> GameSprite::TemplateImage::getRGBAData() {
	auto rgbadata = parent->spriteList[sprite_index]->getRGBAData();
	auto template_rgbdata = parent->spriteList[sprite_index + parent->height * parent->width]->getRGBData();

	if (!rgbadata) {
		spdlog::warn("TemplateImage: Failed to load BASE sprite data for sprite_index={} (template_id={}). Parent width={}, height={}", sprite_index, texture_id, parent->width, parent->height);
		// template_rgbdata auto-deleted
		return nullptr;
	}
	if (!template_rgbdata) {
		spdlog::warn("TemplateImage: Failed to load MASK sprite data for sprite_index={} (template_id={}) (mask_index={})", sprite_index, texture_id, sprite_index + parent->height * parent->width);
		// rgbadata auto-deleted
		return nullptr;
	}

	if (lookHead > TemplateOutfitLookupTableSize) {
		lookHead = 0;
	}
	if (lookBody > TemplateOutfitLookupTableSize) {
		lookBody = 0;
	}
	if (lookLegs > TemplateOutfitLookupTableSize) {
		lookLegs = 0;
	}
	if (lookFeet > TemplateOutfitLookupTableSize) {
		lookFeet = 0;
	}

	Outfit outfit;
	outfit.lookHead = lookHead;
	outfit.lookBody = lookBody;
	outfit.lookLegs = lookLegs;
	outfit.lookFeet = lookFeet;

	const size_t pixel_count = SPRITE_PIXELS * SPRITE_PIXELS;
	const size_t rgba_size = pixel_count * RGBA_COMPONENTS;
	const size_t rgb_size = pixel_count * RGB_COMPONENTS;

	// Note: the base data is RGBA (4 channels) while the mask data is RGB (3 channels).
	GameSprite::ColorizeTemplatePixels(std::span<uint8_t>(rgbadata.get(), rgba_size), std::span<uint8_t>(template_rgbdata.get(), rgb_size), outfit, true);

	// template_rgbdata auto-deleted
	return rgbadata;
}

const AtlasRegion* GameSprite::TemplateImage::getAtlasRegion() {
	if (isGLLoaded && atlas_region) {
		// Self-Healing: Check for stale atlas region pointer
		if (atlas_region->debug_sprite_id == AtlasRegion::INVALID_SENTINEL || (atlas_region->debug_sprite_id != 0 && atlas_region->debug_sprite_id != texture_id)) {
			spdlog::warn("STALE ATLAS REGION DETECTED: TemplateImage {} held region owned by {}. Force reloading.", texture_id, atlas_region->debug_sprite_id);
			isGLLoaded = false;
			atlas_region = nullptr;
		} else {
			visit();
			return atlas_region;
		}
	}

	if (!isGLLoaded) {
		atlas_region = EnsureAtlasSprite(texture_id);
	}
	visit();
	return atlas_region;
}
