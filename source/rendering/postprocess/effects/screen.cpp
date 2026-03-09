#include "rendering/postprocess/post_process_manager.h"

namespace {

	const char* screen_frag = R"(
#version 450 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D u_Texture;

void main() {
    FragColor = texture(u_Texture, vTexCoord);
}
)";

} // namespace

void RegisterScreenEffect(PostProcessManager& mgr) {
	mgr.Register(ShaderNames::NONE, screen_frag);
}
