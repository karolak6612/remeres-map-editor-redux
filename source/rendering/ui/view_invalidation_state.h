#ifndef RME_RENDERING_UI_VIEW_INVALIDATION_STATE_H_
#define RME_RENDERING_UI_VIEW_INVALIDATION_STATE_H_

#include <chrono>
#include <cstdint>
#include <utility>

enum class RepaintReason : uint32_t {
	None = 0,
	ViewportChanged = 1u << 0,
	MapContentChanged = 1u << 1,
	HoverOverlayChanged = 1u << 2,
	InteractionOverlayChanged = 1u << 3,
	AnimationTick = 1u << 4,
};

enum class RefreshScope : uint8_t {
	LocalView,
	SharedMap,
};

constexpr RepaintReason operator|(RepaintReason lhs, RepaintReason rhs) {
	return static_cast<RepaintReason>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr RepaintReason operator&(RepaintReason lhs, RepaintReason rhs) {
	return static_cast<RepaintReason>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr RepaintReason operator~(RepaintReason value) {
	return static_cast<RepaintReason>(~std::to_underlying(value));
}

inline RepaintReason& operator|=(RepaintReason& lhs, RepaintReason rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline RepaintReason& operator&=(RepaintReason& lhs, RepaintReason rhs) {
	lhs = lhs & rhs;
	return lhs;
}

[[nodiscard]] constexpr bool HasFlag(RepaintReason value, RepaintReason flag) {
	return (std::to_underlying(value) & std::to_underlying(flag)) != 0;
}

inline void SetFlag(RepaintReason& value, RepaintReason flag) {
	value |= flag;
}

inline void ClearFlag(RepaintReason& value, RepaintReason flag) {
	value = static_cast<RepaintReason>(std::to_underlying(value) & ~std::to_underlying(flag));
}

struct ViewInvalidationState {
	RepaintReason pending_reasons = RepaintReason::None;
	RefreshScope refresh_scope = RefreshScope::LocalView;
	bool animation_enabled = false;
	bool hover_preview_active = false;
	bool interaction_overlay_active = false;
	bool native_refresh_pending = false;
	double zoom = 1.0;
	std::chrono::steady_clock::time_point last_animation_refresh{};
};

#endif
