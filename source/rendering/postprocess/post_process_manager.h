#ifndef RME_RENDERING_POSTPROCESS_MANAGER_H
#define RME_RENDERING_POSTPROCESS_MANAGER_H

#include <string>
#include <vector>
#include <memory>

class ShaderProgram;

struct PostProcessEffect {
	std::string name;
	std::string fragment_source;
	std::string vertex_source;
	std::shared_ptr<ShaderProgram> shader;

	// Constructor for easy creation
	PostProcessEffect(std::string n, std::string frag, std::string vert = "") : name(n), fragment_source(frag), vertex_source(vert) { }
};

class PostProcessManager {
public:
	PostProcessManager() = default;

	void Register(const std::string& name, const std::string& fragment_source, const std::string& vertex_source = "");
	void Initialize(const std::string& default_vertex_source); // Compiles all registered shaders

	// Loads all pending registrations from EffectRegistry
	void LoadFromRegistry();

	// Returns the shader program for the given name.
	// If not found, returns the first available shader (usually "None") or nullptr.
	ShaderProgram* GetEffect(const std::string& name);

	// Returns a list of all registered effect names (for UI)
	std::vector<std::string> GetEffectNames() const;

private:
	std::vector<std::shared_ptr<PostProcessEffect>> effects;
	bool initialized = false;
};

namespace ShaderNames {
	constexpr const char* NONE = "None";
	constexpr const char* SCANLINE = "Scanline";
	constexpr const char* XBRZ = "4xBRZ";
}

#endif
