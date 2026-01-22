#ifndef RME_MAP_DRAWER_H
#define RME_MAP_DRAWER_H

#include "main.h"
#include "tile.h"
#include "item.h"
#include "brush.h"
#include "editor.h"
#include "gl_wrappers/gl_renderer.h"

class MapCanvas;
class LightDrawer;

struct MapTooltip {
	MapTooltip(int x, int y, std::string text, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) :
		x((float)x),
		y((float)y),
		text(text),
		r(r),
		g(g),
		b(b) {
		////
	}

	void checkLineEnding() {
		// Checks so there are no double line endings
		if (text.size() > 1) {
			if (text[text.size() - 1] == '\n') {
				text = text.substr(0, text.size() - 1);
			}
		}
	}

	static const int MAX_CHARS = 512;
	static const int MAX_CHARS_PER_LINE = 64;

	float x;
	float y;
	std::string text;
	uint8_t r, g, b;
	bool ellipsis = false;
};

struct DrawingOptions {
	DrawingOptions();

	void SetDefault();
	void SetIngame();
	bool isDrawLight() const noexcept;

	bool transparent_floors;
	bool transparent_items;
	bool show_ingame_box;
	bool show_lights;
	bool show_light_str;
	bool show_tech_items;
	bool show_waypoints;
	int show_grid;
	bool ingame;
	bool show_all_floors;
	bool show_creatures;
	bool show_spawns;
	bool show_houses;
	bool show_shade;
	bool show_special_tiles;
	bool show_items;
	bool highlight_items;
	bool highlight_locked_doors;
	bool show_blocking;
	bool show_tooltips;
	bool show_as_minimap;
	bool show_only_colors;
	bool show_only_modified;
	bool show_preview;
	bool show_hooks;
	bool dragging;
	bool hide_items_when_zoomed;
	bool show_towns;
	bool always_show_zones;
	bool extended_house_shader;

	bool experimental_fog = false;
};

class MapDrawer {
public:
	enum BrushColor {
		COLOR_BRUSH,
		COLOR_FLAG_BRUSH,
		COLOR_HOUSE_BRUSH,
		COLOR_SPAWN_BRUSH,
		COLOR_ERASER,
		COLOR_VALID,
		COLOR_INVALID,
	};

	MapDrawer(MapCanvas* canvas);
	~MapDrawer();

	void SetupVars();
	void SetupGL();
	void Release();
	void Draw();

	void DrawBackground();
	void DrawMap();
	void DrawIngameBox();
	void DrawGrid();
	void DrawDraggingShadow();
	void DrawHigherFloors();
	void DrawSelectionBox();
	void DrawLiveCursors();
	void DrawBrush();
	void DrawLight();
	void DrawTooltips();

	void DrawTile(TileLocation* location);

	// Blitting functions
	void BlitItem(int& draw_x, int& draw_y, const Tile* tile, Item* item, bool ephemeral, int red, int green, int blue, int alpha);
	void BlitItem(int& draw_x, int& draw_y, const Position& pos, Item* item, bool ephemeral, int red, int green, int blue, int alpha, const Tile* tile = nullptr);
	void BlitSpriteType(int screenx, int screeny, uint32_t spriteid, int red, int green, int blue, int alpha = 255);
	void BlitSpriteType(int screenx, int screeny, GameSprite* spr, int red, int green, int blue, int alpha = 255);
	void BlitCreature(int screenx, int screeny, const Outfit& outfit, Direction dir, int red, int green, int blue, int alpha = 255);
	void BlitCreature(int screenx, int screeny, const Creature* c, int red = 255, int green = 255, int blue = 255, int alpha = 255);
	void BlitSquare(int sx, int sy, int red, int green, int blue, int alpha, int size = 0);

	void DrawRawBrush(int screenx, int screeny, ItemType* itemType, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
	void DrawBrushIndicator(int x, int y, Brush* brush, uint8_t r, uint8_t g, uint8_t b);
	void DrawHookIndicator(int x, int y, const ItemType& type);

	void WriteTooltip(Item* item, std::ostringstream& stream, bool isHouseTile = false);
	void WriteTooltip(Waypoint* waypoint, std::ostringstream& stream);
	void MakeTooltip(int screenx, int screeny, const std::string& text, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

	void AddLight(TileLocation* location);
	void getColor(Brush* brush, const Position& position, uint8_t& r, uint8_t& g, uint8_t& b);

	void TakeScreenshot(uint8_t* screenshot_buffer);
	DrawingOptions& getOptions() {
		return options;
	}

	void glBlitTexture(int sx, int sy, int texture_number, int red, int green, int blue, int alpha);
	void glBlitSquare(int sx, int sy, int red, int green, int blue, int alpha, int size = 0);

	void glColor(wxColor color);
	void glColor(BrushColor color);
	void glColorCheck(Brush* brush, const Position& pos);

	void drawRect(int x, int y, int w, int h, const wxColor& color, int width = 1);
	void drawFilledRect(int x, int y, int w, int h, const wxColor& color);

protected:
	MapCanvas* canvas;
	Editor& editor;
	std::shared_ptr<LightDrawer> light_drawer;
	BatchRenderer renderer;

	// View variables
	int mouse_map_x, mouse_map_y;
	int view_scroll_x, view_scroll_y, screensize_x, screensize_y;
	int start_x, start_y, end_x, end_y;
	int start_z, end_z, superend_z;

	bool dragging;
	bool dragging_draw;

	float zoom;
	int tile_size;
	int floor;

	int current_house_id;

	DrawingOptions options;

	std::vector<MapTooltip*> tooltips;
	std::ostringstream tooltip;
};

#endif
