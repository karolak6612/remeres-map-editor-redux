//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_GROUND_BRUSH_LOADER_H
#define RME_GROUND_BRUSH_LOADER_H

#include "app/main.h"
#include <vector>

class GroundBrush;
// class std::vector<std::string>;
namespace pugi {
	class xml_node;
}

/**
 * @brief Handles loading of GroundBrush configuration from XML.
 */
class GroundBrushLoader {
public:
	/**
	 * @brief Loads the ground brush configuration.
	 *
	 * @param brush The GroundBrush instance to populate.
	 * @param node The XML node containing configuration.
	 * @param warnings List to append any warnings to.
	 * @return true if loading was successful, false otherwise.
	 */
	static bool load(GroundBrush& brush, pugi::xml_node node, std::vector<std::string>& warnings);

private:
	static bool loadItem(GroundBrush& brush, pugi::xml_node node, std::vector<std::string>& warnings);
	static void loadOptional(GroundBrush& brush, pugi::xml_node node, std::vector<std::string>& warnings);
	static bool loadBorder(GroundBrush& brush, pugi::xml_node node, std::vector<std::string>& warnings);
	static void loadFriend(GroundBrush& brush, pugi::xml_node node, std::vector<std::string>& warnings, bool hate);
	static void clearBorders(GroundBrush& brush);
	static void clearFriends(GroundBrush& brush);
};

#endif // RME_GROUND_BRUSH_LOADER_H
