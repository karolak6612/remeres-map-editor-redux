#ifndef RME_RENDERING_CORE_MAP_ACCESS_H_
#define RME_RENDERING_CORE_MAP_ACCESS_H_

#include <cstdint>

class CopyBuffer;
class LiveSocket;
class Map;
class BaseMap;
class Selection;

class IMapAccess {
public:
	virtual ~IMapAccess() = default;

	[[nodiscard]] virtual Map& getMap() = 0;
	[[nodiscard]] virtual const Map& getMap() const = 0;
	[[nodiscard]] virtual BaseMap& getBaseMap() = 0;
	[[nodiscard]] virtual const BaseMap& getBaseMap() const = 0;
	[[nodiscard]] virtual Selection& getSelection() = 0;
	[[nodiscard]] virtual const Selection& getSelection() const = 0;
	[[nodiscard]] virtual const CopyBuffer& getCopyBuffer() const = 0;
	[[nodiscard]] virtual bool hasMap() const = 0;
	[[nodiscard]] virtual bool isLiveClient() const = 0;
	[[nodiscard]] virtual bool isLive() const = 0;
	[[nodiscard]] virtual LiveSocket* getLiveSocket() = 0;
	[[nodiscard]] virtual const LiveSocket* getLiveSocket() const = 0;
};

#endif
