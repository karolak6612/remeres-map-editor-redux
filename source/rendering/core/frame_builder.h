#ifndef RME_RENDERING_CORE_FRAME_BUILDER_H_
#define RME_RENDERING_CORE_FRAME_BUILDER_H_

#include "rendering/core/brush_snapshot.h"
#include "rendering/core/draw_frame.h"

class Editor;
class AtlasManager;

class FrameBuilder {
public:
	[[nodiscard]] static DrawFrame Build(
		const ViewSnapshot& snapshot,
		const BrushSnapshot& brush,
		const RenderSettings& settings,
		const FrameOptions& base_options,
		const Editor& editor,
		AtlasManager* atlas
	);

private:
	[[nodiscard]] static ViewState ComputeViewState(const ViewSnapshot& snapshot, const RenderSettings& settings);
	[[nodiscard]] static FrameOptions ComputeFrameOptions(
		const FrameOptions& base_options,
		const BrushSnapshot& brush,
		const Editor& editor
	);
};

#endif
