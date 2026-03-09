//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_POSTPROCESS_EFFECTS_H_
#define RME_RENDERING_POSTPROCESS_EFFECTS_H_

#include "rendering/postprocess/post_process_manager.h"
#include <string>
#include <vector>

// Explicit registration functions for each post-process effect.
// These replace the static auto-registration pattern.
void RegisterScreenEffect(PostProcessManager& mgr);
void RegisterScanlineEffect(PostProcessManager& mgr);
void RegisterXBRZEffect(PostProcessManager& mgr);

// Register all built-in post-process effects.
// Call this once during initialization instead of relying on
// static constructor ordering.
inline void RegisterAllPostProcessEffects(PostProcessManager& mgr) {
	RegisterScreenEffect(mgr);
	RegisterScanlineEffect(mgr);
	RegisterXBRZEffect(mgr);
}

// Returns the names of all built-in effects (for UI dropdowns).
// Does not require a PostProcessManager instance.
inline std::vector<std::string> GetAllPostProcessEffectNames() {
	return { ShaderNames::NONE, ShaderNames::SCANLINE, ShaderNames::XBRZ };
}

#endif
