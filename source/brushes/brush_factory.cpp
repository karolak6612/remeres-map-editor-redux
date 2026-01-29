//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "brushes/brush_factory.h"
#include "brushes/brush.h"

BrushFactory& BrushFactory::getInstance() {
	static BrushFactory instance;
	return instance;
}

void BrushFactory::registerBrush(std::string_view type, CreatorFunc creator) {
	creators[std::string(type)] = std::move(creator);
}

Brush* BrushFactory::createBrush(std::string_view type) const {
	if (auto it = creators.find(type); it != creators.end()) {
		return it->second();
	}
	return nullptr;
}
