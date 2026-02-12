#ifndef RME_ICON_RENDERER_H_
#define RME_ICON_RENDERER_H_

#include <nanovg.h>
#include <string>

struct NVGcontext;
struct NVGcolor;

namespace IconRenderer {
	/**
	 * Draws an icon with a 1px black outline.
	 *
	 * @param vg NanoVG context
	 * @param cx Center X coordinate
	 * @param cy Center Y coordinate
	 * @param iconSize Dimensions of the icon (width and height)
	 * @param outlineOffset Offset for the outline highlight
	 * @param iconMacro The icon identifier (macro string)
	 * @param tintColor Color to tint the icon with
	 */
	void DrawIconWithBorder(NVGcontext* vg, float cx, float cy, float iconSize, float outlineOffset, const std::string& iconMacro, const NVGcolor& tintColor);
}

#endif
