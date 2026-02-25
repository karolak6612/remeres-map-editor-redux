#ifndef RME_RENDERING_SYSTEMS_LIVE_SYSTEM_H_
#define RME_RENDERING_SYSTEMS_LIVE_SYSTEM_H_

class Editor;
struct RenderView;

class LiveSystem {
public:
	static void UpdateRequestedNodes(Editor& editor, const RenderView& view, int floor);
};

#endif
