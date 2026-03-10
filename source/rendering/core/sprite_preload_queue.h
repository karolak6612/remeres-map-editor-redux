//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_SPRITE_PRELOAD_QUEUE_H_
#define RME_RENDERING_CORE_SPRITE_PRELOAD_QUEUE_H_

#include <vector>
#include "game/sprites.h"

class GameSprite;

// Buffers sprite preload requests during the render pass so they can be
// batch-processed after frame submission, avoiding per-sprite mutex
// contention in the hot render loop.
struct SpritePreloadQueue {
	struct Request {
		GameSprite* sprite;
		int pattern_x;
		int pattern_y;
		int pattern_z;
		int frame;
	};

	std::vector<Request> requests;

	void enqueue(GameSprite* s, int px, int py, int pz, int f) {
		requests.push_back({ s, px, py, pz, f });
	}

	void processAll() {
		for (const auto& req : requests) {
			rme::collectTileSprites(req.sprite, req.pattern_x, req.pattern_y, req.pattern_z, req.frame);
		}
	}

	void clear() {
		requests.clear();
	}

	void reserve(size_t capacity) {
		requests.reserve(capacity);
	}
};

#endif
