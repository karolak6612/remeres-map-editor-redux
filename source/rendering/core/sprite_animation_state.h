#ifndef RME_RENDERING_CORE_SPRITE_ANIMATION_STATE_H_
#define RME_RENDERING_CORE_SPRITE_ANIMATION_STATE_H_

#include <cstdint>
#include <memory>

class Animator;

struct SpriteAnimationState {
	SpriteAnimationState();
	~SpriteAnimationState();

	SpriteAnimationState(SpriteAnimationState&&) noexcept;
	SpriteAnimationState& operator=(SpriteAnimationState&&) noexcept;

	SpriteAnimationState(const SpriteAnimationState&) = delete;
	SpriteAnimationState& operator=(const SpriteAnimationState&) = delete;

	std::unique_ptr<Animator> animator;
	uint32_t sprite_id = 0;

	[[nodiscard]] int getFrame() const;
	[[nodiscard]] bool hasAnimator() const {
		return animator != nullptr;
	}
};

#endif
