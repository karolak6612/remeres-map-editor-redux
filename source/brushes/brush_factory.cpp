#include "brushes/brush_factory.h"

void BrushFactory::registerBrush(const std::string& typeName, Creator creator) {
	creators[typeName] = std::move(creator);
}

std::unique_ptr<Brush> BrushFactory::createBrush(const std::string& typeName) const {
	if (auto it = creators.find(typeName); it != creators.end()) {
		return it->second();
	}
	return nullptr;
}
