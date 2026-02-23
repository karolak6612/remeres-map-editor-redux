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
#include <atomic>
#include <cstdint>

#include <deque>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <wx/dc.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

enum SpriteSize {
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

	// Helper for SpritePreloader to decompress data off-thread
	[[nodiscard]] static std::unique_ptr<uint8_t[]> Decompress(const uint8_t* dump, size_t size, bool use_alpha, int id = 0);

	static void ColorizeTemplatePixels(uint8_t* dest, const uint8_t* mask, size_t pixel_count, int lookHead, int lookBody, int lookLegs, int lookFeet, bool destHasAlpha);

private:
	static std::unique_ptr<uint8_t[]> DecompressImpl(const uint8_t* dump, size_t size, bool use_alpha, int id);

protected:
	class Image;
	class NormalImage;
	class TemplateImage;

	wxMemoryDC* getDC(SpriteSize size);
	wxMemoryDC* getDC(SpriteSize size, const Outfit& outfit);
	TemplateImage* getTemplateImage(int sprite_index, const Outfit& outfit);

	class Image {
	public:
		Image();
		virtual ~Image() = default;

		bool isGLLoaded = false;
		mutable std::atomic<int64_t> lastaccess;
		uint32_t generation_id = 0;

		void visit() const;
		virtual void clean(time_t time, int longevity);

		virtual std::unique_ptr<uint8_t[]> getRGBData() = 0;
		virtual std::unique_ptr<uint8_t[]> getRGBAData() = 0;

		virtual bool isNormalImage() const {
			return false;
		}

	protected:
		// Helper to handle atlas interactions
		const AtlasRegion* EnsureAtlasSprite(uint32_t sprite_id, std::unique_ptr<uint8_t[]> preloaded_data = nullptr);
	};

	class NormalImage : public Image {
	public:
		NormalImage();
		~NormalImage() override;

		bool isNormalImage() const override {
			return true;
		}

		const AtlasRegion* getAtlasRegion();

		// We use the sprite id as key
		uint32_t id;
		const AtlasRegion* atlas_region;

		// This contains the pixel data
		uint16_t size;
		std::unique_ptr<uint8_t[]> dump;

		void clean(time_t time, int longevity) override;

		std::unique_ptr<uint8_t[]> getRGBData() override;
		std::unique_ptr<uint8_t[]> getRGBAData() override;

		void fulfillPreload(std::unique_ptr<uint8_t[]> preloaded_data);

		GameSprite* parent = nullptr;
	};

	class TemplateImage : public Image {
	public:
		TemplateImage(GameSprite* parent, int v, const Outfit& outfit);
		~TemplateImage() override;

		void clean(time_t time, int longevity) override;

		virtual std::unique_ptr<uint8_t[]> getRGBData() override;
		virtual std::unique_ptr<uint8_t[]> getRGBAData() override;

		const AtlasRegion* getAtlasRegion();
		const AtlasRegion* atlas_region;

		uint32_t texture_id; // Unique ID for AtlasManager key
		GameSprite* parent;
		int sprite_index;
		uint8_t lookHead;
		uint8_t lookBody;
		uint8_t lookLegs;
		uint8_t lookFeet;
	};

	uint32_t id;
	std::unique_ptr<wxMemoryDC> dc[SPRITE_SIZE_COUNT];
	std::unique_ptr<wxBitmap> bm[SPRITE_SIZE_COUNT];

public:
	// GameSprite info
	uint8_t height;
	uint8_t width;
	uint8_t layers;
	uint8_t pattern_x;
	uint8_t pattern_y;
	uint8_t pattern_z;
	uint8_t frames;
	uint32_t numsprites;
	uint32_t getId() const {
		return id;
	}
	uint32_t getDebugImageId(size_t index = 0) const;

	std::unique_ptr<Animator> animator;

	uint16_t draw_height;
	uint16_t drawoffset_x;
	uint16_t drawoffset_y;

	uint16_t minimap_color;

	bool has_light = false;
	SpriteLight light;

	std::vector<NormalImage*> spriteList;
	std::vector<std::unique_ptr<TemplateImage>> instanced_templates; // Templates that use this sprite
	struct CachedDC {
		std::unique_ptr<wxMemoryDC> dc;
		std::unique_ptr<wxBitmap> bm;
	};

	struct RenderKey {
		SpriteSize size;
		uint32_t colorHash;
		uint32_t mountColorHash;
		int lookMount, lookAddon, lookMountHead, lookMountBody, lookMountLegs, lookMountFeet;

		bool operator==(const RenderKey& rk) const {
			return size == rk.size && colorHash == rk.colorHash && mountColorHash == rk.mountColorHash && lookMount == rk.lookMount && lookAddon == rk.lookAddon && lookMountHead == rk.lookMountHead && lookMountBody == rk.lookMountBody && lookMountLegs == rk.lookMountLegs && lookMountFeet == rk.lookMountFeet;
		}
	};

	struct RenderKeyHash {
		size_t operator()(const RenderKey& k) const noexcept {
			// Combine hashes of the most significant fields
			size_t h = std::hash<uint64_t> {}((uint64_t(k.colorHash) << 32) | k.mountColorHash);
			h ^= std::hash<uint64_t> {}((uint64_t(k.lookMount) << 32) | k.lookAddon) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<uint64_t> {}((uint64_t(k.lookMountHead) << 32) | k.lookMountBody) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};
	std::unordered_map<RenderKey, std::unique_ptr<CachedDC>, RenderKeyHash> colored_dc;

	bool is_resident = false; // Tracks if this GameSprite is in resident_game_sprites

	friend class GraphicManager;
	friend class GameSpriteLoader;
	friend class DatLoader;
	friend class SprLoader;
	friend class SpriteIconGenerator;
	friend class TextureGarbageCollector;
	friend class TooltipDrawer;
	friend class SpritePreloader;

	// Exposed for fast-path rendering (BlitItem)
	const AtlasRegion* getCachedDefaultRegion() const {
		return cached_default_region;
	}

	// DEBUG: Get the actual image ID that would be rendered for these coordinates
	uint32_t getSpriteId(int frameIndex, int pattern_x, int pattern_y) const;

	bool isSimpleAndLoaded() const;

	bool is_simple = false;
	void updateSimpleStatus() {
		is_simple = (numsprites == 1 && frames == 1 && layers == 1 && width == 1 && height == 1 && !spriteList.empty());
	}

protected:
	// Cache for default state (0,0,0,0) to avoid lookups/virtual calls for simple sprites
	mutable const AtlasRegion* cached_default_region = nullptr;
	uint32_t cached_generation_id = 0;
	uint32_t cached_sprite_id = 0;
};

#endif
