#include "rendering/core/sprite_icon_data.h"

#include "rendering/core/sprite_icon_renderer.h"
#include "rendering/core/template_image.h"

SpriteIconData::SpriteIconData() = default;
SpriteIconData::~SpriteIconData() = default;

SpriteIconData::SpriteIconData(SpriteIconData&&) noexcept = default;
SpriteIconData& SpriteIconData::operator=(SpriteIconData&&) noexcept = default;

void SpriteIconData::cleanSoftwareData() {
	instanced_templates.clear();
	if (icon_renderer) {
		icon_renderer->unloadDC();
	}
}
