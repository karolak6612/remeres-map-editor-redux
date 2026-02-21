#ifndef RME_RENDERING_CORE_LIGHT_SINK_H_
#define RME_RENDERING_CORE_LIGHT_SINK_H_

struct SpriteLight;

class ILightSink {
public:
	virtual ~ILightSink() = default;

	// Note: We use raw coordinates to match LightBuffer::AddLight
	virtual void AddLight(int map_x, int map_y, int map_z, const SpriteLight& light) = 0;
};

#endif
