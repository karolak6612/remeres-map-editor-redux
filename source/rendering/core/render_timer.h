//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_RENDER_TIMER_H_
#define RME_RENDERING_CORE_RENDER_TIMER_H_

#include "app/main.h"
#include <wx/stopwatch.h>

class RenderTimer {
public:
	RenderTimer();
	~RenderTimer();

	void Start();
	void Pause();
	void Resume();
	long getElapsedTime() const;

private:
	std::unique_ptr<wxStopWatch> timer;
	bool is_paused = false;
};

#endif
