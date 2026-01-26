#include "rendering/drawers/overlays/grid_drawer.h"
#include "ui/gui.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/graphics.h"

#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "app/definitions.h"
#include <wx/gdicmn.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>

const char* grid_vert = R"(
#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Shader for infinite grid
const char* grid_frag = R"(
#version 450 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2 uMapStart; // Map coords (in tiles) at TexCoord 0,0
uniform vec2 uMapEnd;   // Map coords (in tiles) at TexCoord 1,1
uniform float uZoom;

void main() {
    // Interpolate to get map coordinates in tiles
    vec2 mapCoord = mix(uMapStart, uMapEnd, TexCoord);

    // Grid lines are at integer coordinates
    // Use fwidth for anti-aliasing
    vec2 grid = abs(fract(mapCoord - 0.5) - 0.5);
    vec2 derivative = fwidth(mapCoord);

    // 1px width line
    // Screen pixel size in map units = derivative
    // We want line width = 1 pixel

    vec2 line = smoothstep(derivative, vec2(0.0), grid);

    float intensity = max(line.x, line.y);

    if (intensity <= 0.0) discard;

    FragColor = vec4(1.0, 1.0, 1.0, 0.5 * intensity);
}
)";

GridDrawer::GridDrawer() {
}

GridDrawer::~GridDrawer() {
	if (vao_) glDeleteVertexArrays(1, &vao_);
	if (vbo_) glDeleteBuffers(1, &vbo_);
}

bool GridDrawer::initialize() {
	shader_ = std::make_unique<ShaderProgram>();
	if (!shader_->Load(grid_vert, grid_frag)) {
		spdlog::error("GridDrawer: Failed to load shader");
		return false;
	}

	glCreateVertexArrays(1, &vao_);
	glCreateBuffers(1, &vbo_);

	// Unit quad
	float vertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};

	glNamedBufferStorage(vbo_, sizeof(vertices), vertices, 0);

	glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, 4 * sizeof(float));

	glEnableVertexArrayAttrib(vao_, 0);
	glVertexArrayAttribFormat(vao_, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_, 0, 0);

	glEnableVertexArrayAttrib(vao_, 1);
	glVertexArrayAttribFormat(vao_, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(vao_, 1, 0);

	return true;
}

void GridDrawer::DrawGrid(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options) {
	if (!options.show_grid) {
		return;
	}

	if (!shader_) {
		// Fallback or lazy init?
		// Assuming initialized by MapDrawer
		return;
	}

    // We must interrupt the sprite batch if we want to draw with our own shader/VAO
    // This assumes sprite_batch was started.
    // Ideally we should use SpriteBatch if possible, but we want a custom shader.
    // So we flush sprite_batch first.
    if (g_gui.gfx.ensureAtlasManager()) {
        sprite_batch.flush(*g_gui.gfx.getAtlasManager());
    }

	shader_->Use();

	// Calculate geometry
	float xStart = view.start_x * TileSize - view.view_scroll_x - view.getFloorAdjustment();
	float yStart = view.start_y * TileSize - view.view_scroll_y - view.getFloorAdjustment();
	float w = (view.end_x - view.start_x) * TileSize;
	float h = (view.end_y - view.start_y) * TileSize;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(xStart, yStart, 0.0f));
	model = glm::scale(model, glm::vec3(w, h, 1.0f));

	shader_->SetMat4("uMVP", view.projectionMatrix * model);
	shader_->SetVec2("uMapStart", glm::vec2(view.start_x, view.start_y));
	shader_->SetVec2("uMapEnd", glm::vec2(view.end_x, view.end_y));
    shader_->SetFloat("uZoom", view.zoom);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);
}

void GridDrawer::DrawIngameBox(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options) {
	if (!options.show_ingame_box) {
		return;
	}

	int center_x = view.start_x + int(view.screensize_x * view.zoom / 64);
	int center_y = view.start_y + int(view.screensize_y * view.zoom / 64);

	int offset_y = 2;
	int box_start_map_x = center_x;
	int box_start_map_y = center_y + offset_y;
	int box_end_map_x = center_x + ClientMapWidth;
	int box_end_map_y = center_y + ClientMapHeight + offset_y;

	int box_start_x = box_start_map_x * TileSize - view.view_scroll_x;
	int box_start_y = box_start_map_y * TileSize - view.view_scroll_y;
	int box_end_x = box_end_map_x * TileSize - view.view_scroll_x;
	int box_end_y = box_end_map_y * TileSize - view.view_scroll_y;

	static wxColor side_color(0, 0, 0, 200);

	// left side
	if (box_start_map_x >= view.start_x) {
		drawFilledRect(sprite_batch, 0, 0, box_start_x, view.screensize_y * view.zoom, side_color);
	}

	// right side
	if (box_end_map_x < view.end_x) {
		drawFilledRect(sprite_batch, box_end_x, 0, view.screensize_x * view.zoom, view.screensize_y * view.zoom, side_color);
	}

	// top side
	if (box_start_map_y >= view.start_y) {
		drawFilledRect(sprite_batch, box_start_x, 0, box_end_x - box_start_x, box_start_y, side_color);
	}

	// bottom side
	if (box_end_map_y < view.end_y) {
		drawFilledRect(sprite_batch, box_start_x, box_end_y, box_end_x - box_start_x, view.screensize_y * view.zoom, side_color);
	}

	// hidden tiles
	drawRect(sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxRED);

	// visible tiles
	box_start_x += TileSize;
	box_start_y += TileSize;
	box_end_x -= 1 * TileSize;
	box_end_y -= 1 * TileSize;
	drawRect(sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);

	// player position
	box_start_x += (ClientMapWidth - 3) / 2 * TileSize;
	box_start_y += (ClientMapHeight - 3) / 2 * TileSize;
	box_end_x = box_start_x + TileSize;
	box_end_y = box_start_y + TileSize;
	drawRect(sprite_batch, box_start_x, box_start_y, box_end_x - box_start_x, box_end_y - box_start_y, *wxGREEN);
}

void GridDrawer::DrawNodeLoadingPlaceholder(SpriteBatch& sprite_batch, int nd_map_x, int nd_map_y, const RenderView& view) {
	int cy = (nd_map_y)*TileSize - view.view_scroll_y - view.getFloorAdjustment();
	int cx = (nd_map_x)*TileSize - view.view_scroll_x - view.getFloorAdjustment();

	glm::vec4 color(1.0f, 0.0f, 1.0f, 0.5f); // 255, 0, 255, 128

	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_batch.drawRect((float)cx, (float)cy, (float)TileSize * 4, (float)TileSize * 4, color, *g_gui.gfx.getAtlasManager());
	}
}

void GridDrawer::drawRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, int width) {
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_batch.drawRectLines((float)x, (float)y, (float)w, (float)h, c, *g_gui.gfx.getAtlasManager());
	}
}

void GridDrawer::drawFilledRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color) {
	glm::vec4 c(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);

	if (g_gui.gfx.ensureAtlasManager()) {
		sprite_batch.drawRect((float)x, (float)y, (float)w, (float)h, c, *g_gui.gfx.getAtlasManager());
	}
}
