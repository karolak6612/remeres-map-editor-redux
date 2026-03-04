#include "rendering/postprocess/post_process_pipeline.h"
#include "rendering/core/shader_program.h"
#include "rendering/postprocess/post_process_manager.h"
#include "util/file_system.h"
#include <algorithm>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>

PostProcessPipeline::PostProcessPipeline() = default;

PostProcessPipeline::~PostProcessPipeline() = default;

void PostProcessPipeline::Initialize()
{
    if (pp_vao) {
        return;
    }

    // Load Shaders
    wxString exePath = FileSystem::GetExecDirectory();
    std::string fullPath = std::string(exePath.mb_str()) + "/shaders/screen.vert";
    std::ifstream vShaderFile(fullPath);
    std::string default_vert_code;

    if (!vShaderFile.is_open()) {
        spdlog::error("PostProcessPipeline: Failed to load vertex shader from {}", fullPath);
        // Fallback to inline string just in case shaders/ is missing
        default_vert_code = R"(
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";
    } else {
        std::stringstream vStream;
        vStream << vShaderFile.rdbuf();
        default_vert_code = vStream.str();
    }

    PostProcessManager::Instance().Initialize(default_vert_code);

    // Setup Screen Quad
    pp_vao = std::make_unique<GLVertexArray>();
    pp_vbo = std::make_unique<GLBuffer>();
    pp_ebo = std::make_unique<GLBuffer>();

    float quadVertices[] = {// positions   // texCoords
                            -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f
    };

    unsigned int quadIndices[] = {0, 1, 2, 0, 2, 3};

    glNamedBufferStorage(pp_vbo->GetID(), sizeof(quadVertices), quadVertices, 0);
    glNamedBufferStorage(pp_ebo->GetID(), sizeof(quadIndices), quadIndices, 0);

    glVertexArrayVertexBuffer(pp_vao->GetID(), 0, pp_vbo->GetID(), 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(pp_vao->GetID(), pp_ebo->GetID());

    glEnableVertexArrayAttrib(pp_vao->GetID(), 0);
    glVertexArrayAttribFormat(pp_vao->GetID(), 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(pp_vao->GetID(), 0, 0);

    glEnableVertexArrayAttrib(pp_vao->GetID(), 1);
    glVertexArrayAttribFormat(pp_vao->GetID(), 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(pp_vao->GetID(), 1, 0);
}

bool PostProcessPipeline::BeginCapture(const ViewState& view, const DrawingOptions& options)
{
    bool use_fbo = (options.screen_shader_name != ShaderNames::NONE) || options.anti_aliasing;

    if (use_fbo) {
        UpdateFBO(view, options);
        return true;
    }

    return false;
}

void PostProcessPipeline::EndCaptureAndDraw(const ViewState& view, const DrawingOptions& options)
{
    if (!scale_fbo || !pp_vao) {
        return;
    }

    ShaderProgram* shader = PostProcessManager::Instance().GetEffect(options.screen_shader_name);
    if (!shader) {
        return;
    }

    // Only clear and bind main screen once we know we can draw the result
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(view.viewport_x, view.viewport_y, view.screensize_x, view.screensize_y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear main screen

    shader->Use();
    shader->SetInt("u_Texture", 0);
    // Set TextureSize uniform if shader needs it
    shader->SetVec2("u_TextureSize", glm::vec2(fbo_width, fbo_height));

    glBindTextureUnit(0, scale_texture->GetID());
    glBindVertexArray(pp_vao->GetID());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    shader->Unuse();
}

void PostProcessPipeline::UpdateFBO(const ViewState& view, const DrawingOptions& options)
{
    float scale_factor = view.zoom < 1.0f ? view.zoom : 1.0f;

    int target_w = std::max(1, static_cast<int>(view.screensize_x * scale_factor));
    int target_h = std::max(1, static_cast<int>(view.screensize_y * scale_factor));

    bool fbo_resized = false;
    if (fbo_width != target_w || fbo_height != target_h || !scale_fbo) {
        fbo_width = target_w;
        fbo_height = target_h;
        scale_fbo = std::make_unique<GLFramebuffer>();
        scale_texture = std::make_unique<GLTextureResource>(GL_TEXTURE_2D);

        glTextureStorage2D(scale_texture->GetID(), 1, GL_RGBA8, fbo_width, fbo_height);
        glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glNamedFramebufferTexture(scale_fbo->GetID(), GL_COLOR_ATTACHMENT0, scale_texture->GetID(), 0);
        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
        glNamedFramebufferDrawBuffers(scale_fbo->GetID(), 1, drawBuffers);

        if (fbo_width < 1 || fbo_height < 1) {
            spdlog::error("PostProcessPipeline: FBO dimension is zero ({}, {})!", fbo_width, fbo_height);
        }
        fbo_resized = true;
    }

    if (scale_texture && (fbo_resized || options.anti_aliasing != m_lastAaMode)) {
        GLenum filter = options.anti_aliasing ? GL_LINEAR : GL_NEAREST;
        glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_MIN_FILTER, filter);
        glTextureParameteri(scale_texture->GetID(), GL_TEXTURE_MAG_FILTER, filter);
        m_lastAaMode = options.anti_aliasing;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scale_fbo->GetID());
    glViewport(0, 0, fbo_width, fbo_height);
}
