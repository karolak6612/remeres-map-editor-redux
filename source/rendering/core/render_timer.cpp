//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "rendering/core/render_timer.h"

RenderTimer::RenderTimer() {
	timer = std::make_unique<wxStopWatch>();
	timer->Start();
}

RenderTimer::~RenderTimer() {
}

void RenderTimer::Start() {
	timer->Start();
	is_paused = false;
}

void RenderTimer::Pause() {
	if (!is_paused) {
		timer->Pause();
		is_paused = true;
	}
}

void RenderTimer::Resume() {
	if (is_paused) {
		timer->Resume();
		is_paused = false;
	}
}

long RenderTimer::getElapsedTime() const {
	return (timer->TimeInMicro() / 1000).ToLong();
}
