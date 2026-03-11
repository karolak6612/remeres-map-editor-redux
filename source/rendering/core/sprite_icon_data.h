#ifndef RME_RENDERING_CORE_SPRITE_ICON_DATA_H_
#define RME_RENDERING_CORE_SPRITE_ICON_DATA_H_

#include <memory>
#include <vector>

class NormalImage;
class TemplateImage;
class SpriteIconRenderer;

struct SpriteIconData {
	SpriteIconData();
	~SpriteIconData();

	SpriteIconData(SpriteIconData&&) noexcept;
	SpriteIconData& operator=(SpriteIconData&&) noexcept;

	SpriteIconData(const SpriteIconData&) = delete;
	SpriteIconData& operator=(const SpriteIconData&) = delete;

	std::vector<NormalImage*> sprite_list;
	std::vector<std::unique_ptr<TemplateImage>> instanced_templates;
	std::unique_ptr<SpriteIconRenderer> icon_renderer;

	void cleanSoftwareData();
};

#endif
