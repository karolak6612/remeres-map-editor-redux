#include "rendering/core/editor_map_access.h"

#include "editor/editor.h"
#include "live/live_manager.h"

EditorMapAccess::EditorMapAccess(Editor& editor) :
	editor_(editor) {
}

Map& EditorMapAccess::getMap() {
	return editor_.map;
}

const Map& EditorMapAccess::getMap() const {
	return editor_.map;
}

BaseMap& EditorMapAccess::getBaseMap() {
	return editor_.map;
}

const BaseMap& EditorMapAccess::getBaseMap() const {
	return editor_.map;
}

Selection& EditorMapAccess::getSelection() {
	return editor_.selection;
}

const Selection& EditorMapAccess::getSelection() const {
	return editor_.selection;
}

const CopyBuffer& EditorMapAccess::getCopyBuffer() const {
	return editor_.copybuffer;
}

bool EditorMapAccess::hasMap() const {
	return true;
}

bool EditorMapAccess::isLiveClient() const {
	return editor_.live_manager.IsClient();
}

bool EditorMapAccess::isLive() const {
	return editor_.live_manager.IsLive();
}

LiveSocket* EditorMapAccess::getLiveSocket() {
	return editor_.live_manager.IsLive() ? &editor_.live_manager.GetSocket() : nullptr;
}

const LiveSocket* EditorMapAccess::getLiveSocket() const {
	return editor_.live_manager.IsLive() ? &editor_.live_manager.GetSocket() : nullptr;
}
