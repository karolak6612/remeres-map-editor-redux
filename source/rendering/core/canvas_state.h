#ifndef RME_RENDERING_CORE_CANVAS_STATE_H_
#define RME_RENDERING_CORE_CANVAS_STATE_H_

#include "map/position.h"
#include <cstdint>

class BaseMap;

struct CanvasState {
  int last_click_map_x = -1;
  int last_click_map_y = -1;
  int last_click_map_z = -1;

  int last_click_abs_x = -1;
  int last_click_abs_y = -1;

  int cursor_x = -1;
  int cursor_y = -1;

  bool is_dragging_draw = false;
  bool is_pasting = false;

  Position drag_start_pos;

  BaseMap *secondary_map = nullptr;
  uint32_t current_house_id = 0;
};

#endif
