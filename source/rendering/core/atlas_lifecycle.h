#ifndef RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_
#define RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_

#include "rendering/core/atlas_manager.h"
#include <memory>

class AtlasLifecycle {
public:
	AtlasLifecycle() = default;
	~AtlasLifecycle();

	AtlasLifecycle(const AtlasLifecycle&) = delete;
	AtlasLifecycle& operator=(const AtlasLifecycle&) = delete;

	AtlasManager* getAtlasManager() { return atlas_manager_.get(); }
	bool hasAtlasManager() const { return atlas_manager_ != nullptr && atlas_manager_->isValid(); }
	bool ensureAtlasManager();

	void clear();

private:
	std::unique_ptr<AtlasManager> atlas_manager_ = nullptr;
};

#endif
