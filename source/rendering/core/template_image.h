#ifndef RME_RENDERING_CORE_TEMPLATE_IMAGE_H_
#define RME_RENDERING_CORE_TEMPLATE_IMAGE_H_

#include "game/outfit.h"
#include "rendering/core/image.h"

class TemplateImage : public Image {
public:
    TemplateImage(uint32_t clientID, int v, const Outfit& outfit);
    ~TemplateImage() override;

    TemplateImage(const TemplateImage&) = delete;
    TemplateImage& operator=(const TemplateImage&) = delete;

    TemplateImage(TemplateImage&& other) noexcept;
    TemplateImage& operator=(TemplateImage&& other) noexcept;

    void clean(time_t time, int longevity) override;

    virtual std::unique_ptr<uint8_t[]> getRGBData() override;
    virtual std::unique_ptr<uint8_t[]> getRGBAData() override;

    const AtlasRegion* getAtlasRegion(bool block = true);
    const AtlasRegion* atlas_region = nullptr;

    uint32_t texture_id = 0; // Unique ID for AtlasManager key
    uint32_t clientID = 0;
    int sprite_index = 0;
    uint8_t lookHead = 0;
    uint8_t lookBody = 0;
    uint8_t lookLegs = 0;
    uint8_t lookFeet = 0;
};

#endif
