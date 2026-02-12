#include "rendering/utilities/icon_renderer.h"
#include <nanovg.h>
#include "util/image_manager.h"
#include <array>

namespace IconRenderer {

	void DrawIconWithBorder(NVGcontext* vg, float cx, float cy, float iconSize, float outlineOffset, const std::string& iconMacro, const NVGcolor& tintColor) {
		if (!vg) {
			return;
		}

		const wxColour wxTint(
			static_cast<unsigned char>(tintColor.r * 255),
			static_cast<unsigned char>(tintColor.g * 255),
			static_cast<unsigned char>(tintColor.b * 255)
		);
		const wxColour wxBlack(0, 0, 0);

		int imgBlack = IMAGE_MANAGER.GetNanoVGImage(vg, iconMacro, wxBlack);
		int imgTint = IMAGE_MANAGER.GetNanoVGImage(vg, iconMacro, wxTint);

		if (imgBlack > 0 && imgTint > 0) {
			// Modernized outline implementation using std::array and range-based for
			const std::array<std::array<float, 2>, 4> offsets = { { { -outlineOffset, 0.0f }, { outlineOffset, 0.0f }, { 0.0f, -outlineOffset }, { 0.0f, outlineOffset } } };

			const float halfSize = iconSize / 2.0f;

			for (const auto& offset : offsets) {
				float ox = cx + offset[0];
				float oy = cy + offset[1];

				NVGpaint paint = nvgImagePattern(vg, ox - halfSize, oy - halfSize, iconSize, iconSize, 0, imgBlack, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, ox - halfSize, oy - halfSize, iconSize, iconSize);
				nvgFillPaint(vg, paint);
				nvgFill(vg);
			}

			// Draw main icon
			NVGpaint paint = nvgImagePattern(vg, cx - halfSize, cy - halfSize, iconSize, iconSize, 0, imgTint, 1.0f);
			nvgBeginPath(vg);
			nvgRect(vg, cx - halfSize, cy - halfSize, iconSize, iconSize);
			nvgFillPaint(vg, paint);
			nvgFill(vg);
		}
	}

} // namespace IconRenderer
