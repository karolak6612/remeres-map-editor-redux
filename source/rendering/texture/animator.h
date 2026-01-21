//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_ANIMATOR_H_
#define RME_ANIMATOR_H_

#include "common.h"
#include <vector>

enum AnimationDirection {
	ANIMATION_FORWARD = 0,
	ANIMATION_BACKWARD = 1
};

struct FrameDuration {
	int min;
	int max;

	FrameDuration(int min, int max) :
		min(min), max(max) {
		ASSERT(min <= max);
	}

	int getDuration() const {
		if (min == max) {
			return min;
		}
		return uniform_random(min, max);
	};

	void setValues(int min, int max) {
		ASSERT(min <= max);
		this->min = min;
		this->max = max;
	}
};

class Animator {
public:
	Animator(int frames, int start_frame, int loop_count, bool async);
	~Animator();

	int getStartFrame() const;

	FrameDuration* getFrameDuration(int frame);

	int getFrame();
	void setFrame(int frame);

	void reset();

private:
	int getDuration(int frame) const;
	int getPingPongFrame();
	int getLoopFrame();
	void calculateSynchronous();

	int frame_count;
	int start_frame;
	int loop_count;
	bool async;
	std::vector<FrameDuration*> durations;
	int current_frame;
	int current_loop;
	int current_duration;
	int total_duration;
	AnimationDirection direction;
	long last_time;
	bool is_complete;
};

#endif // RME_ANIMATOR_H_
