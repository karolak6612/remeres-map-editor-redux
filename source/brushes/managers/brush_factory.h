#ifndef RME_BRUSH_FACTORY_H
#define RME_BRUSH_FACTORY_H

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string_view>

class Brush;

class BrushFactory {
public:
	using BrushCreator = std::function<std::unique_ptr<Brush>()>;

	static BrushFactory& getInstance();

	void registerBrush(std::string_view typeName, BrushCreator creator);
	std::unique_ptr<Brush> createBrush(std::string_view typeName) const;

	// Helper to auto-register standard brushes
	static void registerStandardBrushes();

private:
	std::unordered_map<std::string, BrushCreator> creators;
};

#endif
