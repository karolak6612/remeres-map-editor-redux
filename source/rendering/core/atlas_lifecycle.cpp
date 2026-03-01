#include "rendering/core/atlas_lifecycle.h"
#include <spdlog/spdlog.h>

AtlasLifecycle::~AtlasLifecycle() {
	clear();
}

void AtlasLifecycle::clear() {
	if (atlas_manager_) {
		atlas_manager_->clear();
		atlas_manager_.reset();
	}
}

bool AtlasLifecycle::ensureAtlasManager() {
	if (atlas_manager_ && atlas_manager_->isValid()) {
		return true;
	}

	if (!atlas_manager_) {
		atlas_manager_ = std::make_unique<AtlasManager>();
	}

	if (!atlas_manager_->ensureInitialized()) {
		spdlog::error("AtlasLifecycle: Failed to initialize atlas manager");
		atlas_manager_.reset();
		return false;
	}

	return true;
}
