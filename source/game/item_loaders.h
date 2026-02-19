#ifndef RME_GAME_ITEM_LOADERS_H
#define RME_GAME_ITEM_LOADERS_H

#include "game/items.h"
#include <vector>
#include <string>

class BinaryNode;
namespace pugi { class xml_node; }

class OtbLoader {
public:
    static bool load(ItemDatabase& db, BinaryNode* itemNode, OtbFileFormatVersion version, wxString& error, std::vector<std::string>& warnings);
};

class XmlLoader {
public:
    static void loadAttributes(ItemType& it, pugi::xml_node node);
};

#endif
