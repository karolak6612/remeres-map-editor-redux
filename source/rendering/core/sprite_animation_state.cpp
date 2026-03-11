#include "rendering/core/sprite_animation_state.h"

#include "rendering/core/animator.h"

SpriteAnimationState::SpriteAnimationState() = default;
SpriteAnimationState::~SpriteAnimationState() = default;

SpriteAnimationState::SpriteAnimationState(SpriteAnimationState&&) noexcept = default;
SpriteAnimationState& SpriteAnimationState::operator=(SpriteAnimationState&&) noexcept = default;

int SpriteAnimationState::getFrame() const {
	return animator ? animator->getFrame() : 0;
}
