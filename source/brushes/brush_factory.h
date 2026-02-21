#pragma once

#include "brushes/brush.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class BrushFactory {
public:
	using Creator = std::function<std::unique_ptr<Brush>()>;

	static BrushFactory& get() {
		static BrushFactory instance;
		return instance;
	}

	void registerBrush(const std::string& typeName, Creator creator);
	std::unique_ptr<Brush> createBrush(const std::string& typeName) const;

private:
	std::unordered_map<std::string, Creator> creators;
	BrushFactory() = default;
};
