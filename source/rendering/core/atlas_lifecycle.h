#ifndef RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_
#define RME_RENDERING_CORE_ATLAS_LIFECYCLE_H_

#include "rendering/core/atlas_manager.h"
#include <memory>
#include <spdlog/spdlog.h>


class AtlasLifecycle {
public:
  AtlasLifecycle() = default;
  ~AtlasLifecycle() {
    if (atlas_manager_) {
      atlas_manager_->clear();
    }
  }

  void clear() {
    if (atlas_manager_) {
      atlas_manager_->clear();
      atlas_manager_.reset();
    }
  }

  AtlasManager *getAtlasManager() { return atlas_manager_.get(); }

  bool hasAtlasManager() const {
    return atlas_manager_ != nullptr && atlas_manager_->isValid();
  }

  bool ensureAtlasManager() {
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

private:
  std::unique_ptr<AtlasManager> atlas_manager_ = nullptr;
};

#endif
