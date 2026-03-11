#ifndef RME_RENDERING_UI_INPUT_STATE_H_
#define RME_RENDERING_UI_INPUT_STATE_H_

// Captures all input-related transient state for the map canvas.
// Reduces MapCanvas private field count and makes input state
// explicitly bundled and passable.
struct InputState {
	int keyCode = 0;
	int cursor_x = 0;
	int cursor_y = 0;
	bool dragging = false;
	bool boundbox_selection = false;
	bool screendragging = false;

	int last_cursor_map_x = 0;
	int last_cursor_map_y = 0;
	int last_cursor_map_z = 0;
	int last_click_map_x = 0;
	int last_click_map_y = 0;
	int last_click_map_z = 0;
	int last_click_abs_x = 0;
	int last_click_abs_y = 0;
	int last_click_x = 0;
	int last_click_y = 0;
	int last_mmb_click_x = 0;
	int last_mmb_click_y = 0;
};

#endif
