//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/brush_factory.h"
#include "brushes/brush.h"

BrushFactory& BrushFactory::getInstance() {
	static BrushFactory instance;
	return instance;
}

void BrushFactory::registerBrush(std::string_view type, Creator creator) {
	creators.emplace(type, std::move(creator));
}

std::unique_ptr<Brush> BrushFactory::createBrush(std::string_view type) const {
	if (auto it = creators.find(std::string(type)); it != creators.end()) {
		return it->second();
	}
	return nullptr;
}

void BrushFactory::clear() {
	creators.clear();
}
