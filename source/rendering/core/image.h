#ifndef RME_RENDERING_CORE_IMAGE_H_
#define RME_RENDERING_CORE_IMAGE_H_

#include <atomic>
#include <memory>
#include <cstdint>
#include "rendering/core/atlas_manager.h" // For AtlasRegion

enum class ImageType : uint8_t {
    None = 0,
    Normal,
    Template
};

struct ImageHandle {
    ImageType type = ImageType::None;
    uint32_t index = 0;
    uint32_t generation = 0;

    bool isValid() const { return type != ImageType::None; }

    bool operator==(const ImageHandle& other) const {
        return type == other.type && index == other.index && generation == other.generation;
    }
};

class TextureGC;
class AtlasManager;
class SpriteLoader;
class SpriteDatabase;

class Image {
public:
	Image();
	virtual ~Image() = default;

	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;

	Image(Image&& other) noexcept;
	Image& operator=(Image&& other) noexcept;

	ImageHandle handle;
	bool isGLLoaded = false;
	mutable std::atomic<int64_t> lastaccess;
	uint32_t generation_id = 0;

	void visit(TextureGC& gc) const;
	virtual void clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc);

	virtual std::unique_ptr<uint8_t[]> getRGBData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached) = 0;
	virtual std::unique_ptr<uint8_t[]> getRGBAData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached) = 0;

	virtual bool isNormalImage() const {
		return false;
	}

protected:
	// Helper to handle atlas interactions
	const AtlasRegion* EnsureAtlasSprite(SpriteDatabase* sprites, AtlasManager& atlas_mgr, TextureGC& gc, SpriteLoader& loader, bool use_memcached, uint32_t sprite_id, std::unique_ptr<uint8_t[]> preloaded_data = nullptr);
};

#endif
