#ifndef RME_RENDERING_EFFECT_REGISTRY_H_
#define RME_RENDERING_EFFECT_REGISTRY_H_

#include <string>
#include <vector>

struct EffectRegistration {
	std::string name;
	std::string fragment_source;
	std::string vertex_source;
};

// Global effect registry for static auto-registration of post-process effects.
// Effects register themselves during static initialization (before main).
// PostProcessManager consumes these registrations during Initialize().
namespace EffectRegistry {

inline std::vector<EffectRegistration>& Pending() {
	static std::vector<EffectRegistration> registrations;
	return registrations;
}

inline void Register(std::string name, std::string frag, std::string vert = "") {
	Pending().push_back({ std::move(name), std::move(frag), std::move(vert) });
}

inline std::vector<std::string> GetRegisteredNames() {
	std::vector<std::string> names;
	for (const auto& reg : Pending()) {
		names.push_back(reg.name);
	}
	return names;
}

} // namespace EffectRegistry

#endif
