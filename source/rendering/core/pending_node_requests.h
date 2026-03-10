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

#ifndef RME_RENDERING_CORE_PENDING_NODE_REQUESTS_H_
#define RME_RENDERING_CORE_PENDING_NODE_REQUESTS_H_

#include <vector>
#include <mutex>

// Thread-safe buffer for live-client node requests generated during
// the render pass. Requests are enqueued by MapLayerDrawer and drained
// by MapDrawer after Draw() completes, keeping network I/O off the
// GPU submission path.
class PendingNodeRequests {
public:
	struct NodeRequest {
		int x;
		int y;
		bool underground;
	};

	void enqueue(int x, int y, bool underground) {
		std::lock_guard<std::mutex> lock(mutex_);
		pending_.push_back({ x, y, underground });
	}

	// Returns all pending requests and clears the internal buffer.
	std::vector<NodeRequest> drain() {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<NodeRequest> result;
		result.swap(pending_);
		return result;
	}

	void reserve(size_t capacity) {
		std::lock_guard<std::mutex> lock(mutex_);
		pending_.reserve(capacity);
	}

private:
	std::vector<NodeRequest> pending_;
	std::mutex mutex_;
};

#endif
