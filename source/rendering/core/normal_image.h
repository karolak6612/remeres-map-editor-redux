#ifndef RME_RENDERING_CORE_NORMAL_IMAGE_H_
#define RME_RENDERING_CORE_NORMAL_IMAGE_H_

#include "rendering/core/image.h"

class GameSprite;

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

#endif
