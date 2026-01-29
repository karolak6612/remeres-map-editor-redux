//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_BRUSH_FACTORY_H_
#define RME_BRUSH_FACTORY_H_

#include <string_view>
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>

class Brush;

class BrushFactory {
public:
	using CreatorFunc = std::function<Brush*()>;

	static BrushFactory& getInstance();

	void registerBrush(std::string_view type, CreatorFunc creator);
	Brush* createBrush(std::string_view type) const;

private:
	struct StringHash {
		using is_transparent = void;
		size_t operator()(const char* txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		size_t operator()(const std::string& txt) const {
			return std::hash<std::string> {}(txt);
		}
	};

	BrushFactory() = default;
	std::unordered_map<std::string, CreatorFunc, StringHash, std::equal_to<>> creators;
};

#endif
