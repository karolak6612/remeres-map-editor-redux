#ifndef RME_INGAME_PREVIEW_CANVAS_H_
#define RME_INGAME_PREVIEW_CANVAS_H_

#include "app/main.h"
#include "map/position.h"
#include "game/creature.h"
#include "game/outfit.h"
#include <memory>

class Editor;

#include <deque>
#include <string>
#include "rendering/core/graphics.h"
#include "util/nanovg_canvas.h"

struct NVGcontext;

namespace IngamePreview {

	class IngamePreviewRenderer;

	class IngamePreviewCanvas : public NanoVGCanvas {
	public:
		IngamePreviewCanvas(wxWindow* parent);
		~IngamePreviewCanvas();

		// Hooks from NanoVGCanvas
		void OnGLPaint() override;
		void OnNanoVGPaint(NVGcontext* vg, int width, int height) override;
		bool ShouldClearBackground() const override { return false; } // Renderer clears or fills

		void OnSize(wxSizeEvent& event);
		void OnMouseMove(wxMouseEvent& event);
		void OnKeyDown(wxKeyEvent& event);
		void OnTimer(wxTimerEvent& event);
		void OnEraseBackground(wxEraseEvent& event) { }

		void SetCameraPosition(const Position& pos);
		void SetZoom(float z);
		void SetLightingEnabled(bool enabled);
		void SetAmbientLight(uint8_t ambient);
		void SetLightIntensity(float intensity);
		void SetPreviewOutfit(const Outfit& outfit) {
			preview_outfit = outfit;
			Refresh();
		}
		void SetViewportSize(int w, int h);
		void GetViewportSize(int& w, int& h) const;

		void SetName(const std::string& name) {
			preview_name_str = name;
			Refresh();
		}
		void SetSpeed(uint16_t s) {
			speed = s;
			Refresh();
		}

		bool IsWalking() const {
			return is_walking;
		}

	private:
		void Render(Editor* current_editor);

		std::unique_ptr<IngamePreviewRenderer> renderer;
		const void* last_tile_renderer;

		Position camera_pos;
		float zoom;
		bool lighting_enabled;
		uint8_t ambient_light;
		float light_intensity;

		int viewport_width_tiles;
		int viewport_height_tiles;

		Outfit preview_outfit;
		Direction preview_direction;
		std::string preview_name_str;

		// Movement state
		bool is_walking;
		long long walk_start_time; // Using wxGetLocalTimeMillis()
		int walk_duration;
		Direction walk_direction;
		Direction next_walk_direction;
		Position walk_source_pos;
		int walk_offset_x;
		int walk_offset_y;
		int animation_phase;

		// New Movement Support
		struct MoveRequest {
			Direction dir;
			bool ignore_collision;
		};
		std::deque<MoveRequest> walk_queue;
		uint16_t speed;
		long long last_step_time;
		long long walk_lock_timer;

		wxTimer animation_timer;

		void UpdateWalk();
		void StartWalk(Direction dir, bool ignore_collision = false);
		// New helpers
		void BufferWalk(Direction dir, bool ignore_collision = false);
		bool CanWalk(const Position& target_pos, bool ignore_collision);
		int GetStepDuration(uint16_t ground_speed = 100);
		void Turn(Direction dir);
	};

} // namespace IngamePreview

#endif // RME_INGAME_PREVIEW_CANVAS_H_
