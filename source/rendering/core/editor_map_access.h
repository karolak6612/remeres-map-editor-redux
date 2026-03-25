#ifndef RME_RENDERING_CORE_EDITOR_MAP_ACCESS_H_
#define RME_RENDERING_CORE_EDITOR_MAP_ACCESS_H_

#include "rendering/core/map_access.h"

class Editor;

class EditorMapAccess : public IMapAccess {
public:
	explicit EditorMapAccess(Editor& editor);

	Map& getMap() override;
	const Map& getMap() const override;
	BaseMap& getBaseMap() override;
	const BaseMap& getBaseMap() const override;
	Selection& getSelection() override;
	const Selection& getSelection() const override;
	const CopyBuffer& getCopyBuffer() const override;
	bool hasMap() const override;
	bool isLiveClient() const override;
	bool isLive() const override;
	LiveSocket* getLiveSocket() override;
	const LiveSocket* getLiveSocket() const override;

private:
	Editor& editor_;
};

#endif
