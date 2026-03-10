//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_GAME_SPRITE_H_
#define RME_RENDERING_CORE_GAME_SPRITE_H_

#include "game/outfit.h"
#include "util/common.h"
#include "rendering/core/animator.h"
#include "rendering/core/sprite_light.h"
#include "rendering/core/texture_garbage_collector.h"
#include "rendering/core/atlas_manager.h"
#include "rendering/core/render_timer.h"
#include "rendering/core/sprite_metadata.h"
#include "rendering/core/sprite_decompression.h"
#include "rendering/core/sprite_icon_renderer.h"
#include <atomic>
#include <cstdint>
#include <span>

#include <deque>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <wx/dc.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

enum SpriteSize : int {
	SPRITE_SIZE_16x16,
	// SPRITE_SIZE_24x24,
	SPRITE_SIZE_32x32,
	SPRITE_SIZE_64x64,
	SPRITE_SIZE_COUNT
};

class GraphicManager;
class SpritePreloader;

class Sprite {
public:
	Sprite() { }
	virtual ~Sprite() = default;

	virtual void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) = 0;
	virtual void unloadDC() = 0;
	virtual wxSize GetSize() const = 0;

private:
	Sprite(const Sprite&);
	Sprite& operator=(const Sprite&);
};

class GameSprite;
class CreatureSprite : public Sprite {
public:
	CreatureSprite(GameSprite* parent, const Outfit& outfit);
	~CreatureSprite() override;

	void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) override;
	virtual void unloadDC() override;
	wxSize GetSize() const override {
		return wxSize(32, 32);
	}

	GameSprite* parent;
	Outfit outfit;
};

class Image;
class NormalImage;
class TemplateImage;

class GameSprite : public Sprite {
public:
	GameSprite();
	~GameSprite() override;

	size_t getIndex(int width, int height, int layer, int pattern_x, int pattern_y, int pattern_z, int frame) const;

	// Phase 2: Get atlas region for texture array rendering
	const AtlasRegion* getAtlasRegion(int _x, int _y, int _layer, int _subtype, int _pattern_x, int _pattern_y, int _pattern_z, int _frame);
	const AtlasRegion* getAtlasRegion(int _x, int _y, int _dir, int _addon, int _pattern_z, const Outfit& _outfit, int _frame);

	void DrawTo(wxDC* dc, SpriteSize sz, int start_x, int start_y, int width = -1, int height = -1) override;
	virtual void DrawTo(wxDC* dc, SpriteSize sz, const Outfit& outfit, int start_x, int start_y, int width = -1, int height = -1);

	void unloadDC() override;

	wxSize GetSize() const override {
		return wxSize(width * 32, height * 32);
	}

	void clean(time_t time, int longevity = -1);

	int getDrawHeight() const;
	std::pair<int, int> getDrawOffset() const;
	uint8_t getMiniMapColor() const;

	bool hasLight() const noexcept {
		return has_light;
	}
	const SpriteLight& getLight() const noexcept {
		return light;
	}

	// Thin wrappers delegating to SpriteDecompression namespace
	[[nodiscard]] static std::unique_ptr<uint8_t[]> Decompress(std::span<const uint8_t> dump, bool use_alpha, int id = 0) {
		return SpriteDecompression::Decompress(dump, use_alpha, id);
	}

	static void ColorizeTemplatePixels(uint8_t* dest, const uint8_t* mask, size_t pixelCount, int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha) {
		SpriteDecompression::ColorizeTemplatePixels(dest, mask, pixelCount, lookHead, lookBody, lookLegs, lookFeet, destHasAlpha);
	}

	// Exposed for NormalImage::clean to invalidate cache
	void invalidateCache(const AtlasRegion* region);

	SpriteIconRenderer& iconRenderer() { return icon_renderer_; }

protected:
	TemplateImage* getTemplateImage(int sprite_index, const Outfit& outfit);

public:
	// Sprite metadata (public for GraphicsAssembler direct writes)
	uint32_t id = 0;
	uint8_t height = 0;
	uint8_t width = 0;
	uint8_t layers = 0;
	uint8_t pattern_x = 0;
	uint8_t pattern_y = 0;
	uint8_t pattern_z = 0;
	uint8_t frames = 0;
	uint32_t numsprites = 0;

	uint32_t getId() const { return id; }
	uint32_t getDebugImageId(size_t index = 0) const;

	std::unique_ptr<Animator> animator;

	uint16_t draw_height = 0;
	uint16_t drawoffset_x = 0;
	uint16_t drawoffset_y = 0;

	uint16_t minimap_color = 0;

	bool has_light = false;
	SpriteLight light;

	std::vector<NormalImage*> spriteList;
	std::vector<std::unique_ptr<TemplateImage>> instanced_templates;

	bool is_resident = false;

	friend class GraphicManager;
	friend class GraphicsAssembler;
	friend class SpriteIconGenerator;
	friend class TextureGarbageCollector;
	friend class SpritePreloader;

	// Exposed for fast-path rendering (BlitItem)
	const AtlasRegion* getCachedDefaultRegion() const {
		return cached_default_region;
	}

	uint32_t getSpriteId(int frameIndex, int pattern_x, int pattern_y) const;

	bool isSimpleAndLoaded() const;

	bool is_simple = false;
	void updateSimpleStatus() {
		is_simple = (numsprites == 1 && frames == 1 && layers == 1 && width == 1 && height == 1 && !spriteList.empty());
	}

protected:
	mutable const AtlasRegion* cached_default_region = nullptr;
	uint32_t cached_generation_id = 0;
	uint32_t cached_sprite_id = 0;

private:
	SpriteIconRenderer icon_renderer_;
};

#endif
