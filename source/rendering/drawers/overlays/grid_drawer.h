#ifndef RME_RENDERING_GRID_DRAWER_H_
#define RME_RENDERING_GRID_DRAWER_H_

#include "app/main.h"
#include <wx/colour.h>
#include <memory>
#include "rendering/core/shader_program.h"

struct RenderView;
struct DrawingOptions;
class SpriteBatch;

class GridDrawer {
public:
	GridDrawer();
	~GridDrawer();

	bool initialize();

	void DrawGrid(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options);
	void DrawIngameBox(SpriteBatch& sprite_batch, const RenderView& view, const DrawingOptions& options);
	void DrawNodeLoadingPlaceholder(SpriteBatch& sprite_batch, int nd_map_x, int nd_map_y, const RenderView& view);

private:
	void drawRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color, int width = 1);
	void drawFilledRect(SpriteBatch& sprite_batch, int x, int y, int w, int h, const wxColor& color);

	std::unique_ptr<ShaderProgram> shader_;
	GLuint vao_ = 0;
	GLuint vbo_ = 0;
};

#endif
