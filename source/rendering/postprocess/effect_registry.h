#ifndef RME_RENDERING_EFFECT_REGISTRY_H_
#define RME_RENDERING_EFFECT_REGISTRY_H_

// Built-in post-process effect shader sources.
// Implementations live in rendering/postprocess/effects/*.cpp and are
// registered explicitly by PostProcessManager.
namespace EffectRegistry {

const char* ScreenFragmentSource();
const char* ScanlineFragmentSource();
const char* XbrzFragmentSource();
const char* XbrzVertexSource();

} // namespace EffectRegistry

#endif
