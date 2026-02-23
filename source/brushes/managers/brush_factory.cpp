#include "app/main.h"
#include "brushes/managers/brush_factory.h"
#include "brushes/brush.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "brushes/carpet/carpet_brush.h"
#include "brushes/table/table_brush.h"
#include "brushes/doodad/doodad_brush.h"

BrushFactory& BrushFactory::getInstance() {
	static BrushFactory instance;
	return instance;
}

void BrushFactory::registerBrush(std::string_view typeName, BrushCreator creator) {
	creators[std::string(typeName)] = std::move(creator);
}

std::unique_ptr<Brush> BrushFactory::createBrush(std::string_view typeName) const {
	auto it = creators.find(std::string(typeName));
	if (it != creators.end()) {
		return it->second();
	}
	return nullptr;
}

void BrushFactory::registerStandardBrushes() {
	auto& factory = getInstance();
	factory.registerBrush("border", [] { return std::make_unique<GroundBrush>(); });
	factory.registerBrush("ground", [] { return std::make_unique<GroundBrush>(); });
	factory.registerBrush("wall", [] { return std::make_unique<WallBrush>(); });
	factory.registerBrush("wall decoration", [] { return std::make_unique<WallDecorationBrush>(); });
	factory.registerBrush("carpet", [] { return std::make_unique<CarpetBrush>(); });
	factory.registerBrush("table", [] { return std::make_unique<TableBrush>(); });
	factory.registerBrush("doodad", [] { return std::make_unique<DoodadBrush>(); });
}
