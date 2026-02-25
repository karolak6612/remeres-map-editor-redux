#include "rendering/systems/live_system.h"
#include "editor/editor.h"
#include "live/live_client.h"
#include "live/live_manager.h"
#include "rendering/core/render_view.h"
#include "map/map.h"
#include "map/map_region.h"

void LiveSystem::UpdateRequestedNodes(Editor& editor, const RenderView& view, int floor) {
	if (!editor.live_manager.IsClient()) {
		return;
	}

	LiveClient* client = editor.live_manager.GetClient();
	if (!client) {
		return;
	}

	int nd_start_x = view.start_x & ~3;
	int nd_start_y = view.start_y & ~3;
	int nd_end_x = (view.end_x & ~3) + 4;
	int nd_end_y = (view.end_y & ~3) + 4;

	// Loop through visible nodes and request them if needed
	// This logic duplicates the traversal in MapLayerDrawer but runs in Update
	// to avoid side effects during rendering.

	bool underground = floor > GROUND_LAYER;
	// When viewing underground, we need to request underground nodes.
	// When viewing above ground, we need to request above ground nodes.
	// The 'floor' parameter passed here is the specific layer being drawn?
	// Or the current camera floor?
	// MapLayerDrawer iterates layers.

	// Actually, MapLayerDrawer requests based on "map_z > GROUND_LAYER".
	// Since we want to pre-fetch everything visible, we should iterate the Z range visible in the view.

	for (int map_z = view.start_z; map_z >= view.superend_z; map_z--) {
		bool layer_underground = map_z > GROUND_LAYER;

		for (int nd_map_x = nd_start_x; nd_map_x <= nd_end_x; nd_map_x += 4) {
			for (int nd_map_y = nd_start_y; nd_map_y <= nd_end_y; nd_map_y += 4) {
				MapNode* nd = editor.map.getLeaf(nd_map_x, nd_map_y);
				if (!nd) {
					// Logic from MapLayerDrawer: create if missing?
					// "if (!nd) nd = editor->map.createLeaf..."
					// We shouldn't create leaves just for querying, but if MapLayerDrawer does it...
					// Let's stick to existing behavior: if it's visible, we want it.
					nd = editor.map.createLeaf(nd_map_x, nd_map_y);
					nd->setVisible(false, false);
				}

				if (!nd->isVisible(layer_underground)) {
					if (!nd->isRequested(layer_underground)) {
						client->queryNode(nd_map_x, nd_map_y, layer_underground);
						nd->setRequested(layer_underground, true);
					}
				}
			}
		}
	}
}
