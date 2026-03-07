#ifndef RME_RENDERING_CORE_NORMAL_IMAGE_H_
#define RME_RENDERING_CORE_NORMAL_IMAGE_H_

#include "rendering/core/image.h"

class NormalImage : public Image {
public:
    NormalImage();
    ~NormalImage() override;

    NormalImage(const NormalImage&) = delete;
    NormalImage& operator=(const NormalImage&) = delete;

    NormalImage(NormalImage&& other) noexcept;
    NormalImage& operator=(NormalImage&& other) noexcept;

    bool isNormalImage() const override
    {
        return true;
    }

    const AtlasRegion* getAtlasRegion(SpriteDatabase& sprites, AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, bool block = true);

    // We use the sprite id as key
    uint32_t id = 0;
    const AtlasRegion* atlas_region = nullptr;

    // This contains the pixel data
    uint16_t size = 0;
    std::unique_ptr<uint8_t[]> dump;

    void clean(time_t time, int longevity, SpriteDatabase& sprites, TextureGC& gc) override;

    std::unique_ptr<uint8_t[]> getRGBData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached) override;
    std::unique_ptr<uint8_t[]> getRGBAData(SpriteDatabase* sprites, SpriteLoader& loader, bool use_memcached) override;

    void fulfillPreload(AtlasManager& atlas, TextureGC& gc, SpriteLoader& loader, bool use_memcached, std::unique_ptr<uint8_t[]> preloaded_data);

    bool ensureDumpLoaded(SpriteLoader& loader, bool use_memcached);
    void unloadGL(AtlasManager* atlas, TextureGC& gc);

    uint32_t clientID = 0;
};

#endif
