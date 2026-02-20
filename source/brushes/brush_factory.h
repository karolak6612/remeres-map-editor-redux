//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_BRUSH_FACTORY_H_
#define RME_BRUSH_FACTORY_H_

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <string_view>

class Brush;

class BrushFactory {
public:
	using Creator = std::function<std::unique_ptr<Brush>()>;

	static BrushFactory& getInstance();

	void registerBrush(std::string_view type, Creator creator);
	std::unique_ptr<Brush> createBrush(std::string_view type) const;

	// For removing all registered brushes (useful for tests or re-init)
	void clear();

private:
	BrushFactory() = default;
	~BrushFactory() = default;

	std::unordered_map<std::string, Creator> creators;
};

#endif
