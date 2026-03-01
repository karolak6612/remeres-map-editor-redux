//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"

#include "brushes/brush.h"
#include "brushes/creature/creature_brush.h"
#include "game/creatures.h"
#include "game/materials.h"
#include "ui/gui.h"
#include <algorithm>

CreatureDatabase g_creatures;

CreatureType::CreatureType() : isNpc(false), missing(false), in_other_tileset(false), standard(false), brush(nullptr)
{
    ////
}

CreatureType::CreatureType(const CreatureType& ct) :
    isNpc(ct.isNpc),
    missing(ct.missing),
    in_other_tileset(ct.in_other_tileset),
    standard(ct.standard),
    name(ct.name),
    outfit(ct.outfit),
    brush(ct.brush)
{
    ////
}

CreatureType& CreatureType::operator=(const CreatureType& ct)
{
    if (this == &ct) {
        return *this;
    }
    isNpc = ct.isNpc;
    missing = ct.missing;
    in_other_tileset = ct.in_other_tileset;
    standard = ct.standard;
    name = ct.name;
    outfit = ct.outfit;
    brush = ct.brush;
    return *this;
}

CreatureType::~CreatureType()
{
    ////
}

void CreatureType::preserve_assign_creature_fields(CreatureType* dest, const CreatureType& src)
{
    CreatureBrush* oldBrush = dest->brush;
    bool oldInTileset = dest->in_other_tileset;
    bool oldStandard = dest->standard;

    *dest = src;

    dest->brush = oldBrush;
    dest->in_other_tileset = oldInTileset;
    dest->standard = oldStandard;
}

CreatureType* CreatureType::loadFromXML(pugi::xml_node node, std::vector<std::string>& warnings)
{
    pugi::xml_attribute attribute = node.attribute("type");
    if (!attribute) {
        warnings.push_back("Couldn't read type tag of creature node.");
        return nullptr;
    }

    const std::string& tmpType = attribute.as_string();
    if (tmpType != "monster" && tmpType != "npc") {
        warnings.push_back((wxString("Invalid type tag of creature node \"") + wxstr(tmpType) + "\"").ToStdString());
        return nullptr;
    }

    attribute = node.attribute("name");
    if (!attribute) {
        warnings.push_back("Couldn't read name tag of creature node.");
        return nullptr;
    }

    auto ct = std::make_unique<CreatureType>();
    ct->name = attribute.as_string();
    ct->isNpc = tmpType == "npc";

    attribute = node.attribute("looktype");
    if (attribute) {
        ct->outfit.lookType = attribute.as_int();

        if (g_gui.sprites.getCreatureSprite(ct->outfit.lookType) == nullptr) {
            warnings.push_back(
                (wxString("Invalid creature \"") + wxstr(ct->name) + "\" look type #" + std::to_string(ct->outfit.lookType)).ToStdString()
            );
        }
    } else {
        // Log if no looktype is present (defaulting to 0)
    }

    attribute = node.attribute("lookitem");
    if (attribute) {
        ct->outfit.lookItem = attribute.as_int();
    }

    attribute = node.attribute("lookmount");
    if (attribute) {
        ct->outfit.lookMount = attribute.as_int();
    }

    attribute = node.attribute("lookaddon");
    if (attribute) {
        ct->outfit.lookAddon = attribute.as_int();
    }

    attribute = node.attribute("lookhead");
    if (attribute) {
        ct->outfit.lookHead = attribute.as_int();
    }

    attribute = node.attribute("lookbody");
    if (attribute) {
        ct->outfit.lookBody = attribute.as_int();
    }

    attribute = node.attribute("looklegs");
    if (attribute) {
        ct->outfit.lookLegs = attribute.as_int();
    }

    attribute = node.attribute("lookfeet");
    if (attribute) {
        ct->outfit.lookFeet = attribute.as_int();
    }

    attribute = node.attribute("lookmounthead");
    if (attribute) {
        ct->outfit.lookMountHead = attribute.as_int();
    }

    attribute = node.attribute("lookmountbody");
    if (attribute) {
        ct->outfit.lookMountBody = attribute.as_int();
    }

    attribute = node.attribute("lookmountlegs");
    if (attribute) {
        ct->outfit.lookMountLegs = attribute.as_int();
    }

    attribute = node.attribute("lookmountfeet");
    if (attribute) {
        ct->outfit.lookMountFeet = attribute.as_int();
    }

    return ct.release();
}

CreatureType* CreatureType::loadFromOTXML(const FileName& filename, pugi::xml_document& doc, std::vector<std::string>& warnings)
{

    bool isNpc = false;
    pugi::xml_node node = doc.child("monster");
    if (node) {
        isNpc = false;
    } else {
        node = doc.child("npc");
        if (node) {
            isNpc = true;
        } else {
            warnings.push_back("This file is not a monster/npc file");
            return nullptr;
        }
    }

    pugi::xml_attribute attribute;
    attribute = node.attribute("name");
    if (!attribute) {
        warnings.push_back("Couldn't read name tag of creature node.");
        return nullptr;
    }

    auto ct = std::make_unique<CreatureType>();
    if (isNpc) {
        ct->name = nstr(filename.GetName());
    } else {
        ct->name = attribute.as_string();
    }
    ct->isNpc = isNpc;

    for (pugi::xml_node optionNode = node.first_child(); optionNode; optionNode = optionNode.next_sibling()) {
        if (as_lower_str(optionNode.name()) != "look") {
            continue;
        }

        attribute = optionNode.attribute("type");
        if (attribute) {
            ct->outfit.lookType = attribute.as_int();
        }

        attribute = optionNode.attribute("item");
        if (!attribute) {
            attribute = optionNode.attribute("lookex");
        }
        if (!attribute) {
            attribute = optionNode.attribute("typeex");
        }

        if (attribute) {
            ct->outfit.lookItem = attribute.as_int();
        }

        attribute = optionNode.attribute("mount");
        if (attribute) {
            ct->outfit.lookMount = attribute.as_int();
        }

        attribute = optionNode.attribute("addon");
        if (attribute) {
            ct->outfit.lookAddon = attribute.as_int();
        }

        attribute = optionNode.attribute("head");
        if (attribute) {
            ct->outfit.lookHead = attribute.as_int();
        }

        attribute = optionNode.attribute("body");
        if (attribute) {
            ct->outfit.lookBody = attribute.as_int();
        }

        attribute = optionNode.attribute("legs");
        if (attribute) {
            ct->outfit.lookLegs = attribute.as_int();
        }

        attribute = optionNode.attribute("feet");
        if (attribute) {
            ct->outfit.lookFeet = attribute.as_int();
        }

        attribute = optionNode.attribute("mounthead");
        if (attribute) {
            ct->outfit.lookMountHead = attribute.as_int();
        }

        attribute = optionNode.attribute("mountbody");
        if (attribute) {
            ct->outfit.lookMountBody = attribute.as_int();
        }

        attribute = optionNode.attribute("mountlegs");
        if (attribute) {
            ct->outfit.lookMountLegs = attribute.as_int();
        }

        attribute = optionNode.attribute("mountfeet");
        if (attribute) {
            ct->outfit.lookMountFeet = attribute.as_int();
        }
    }
    return ct.release();
}

CreatureDatabase::CreatureDatabase()
{
    ////
}

CreatureDatabase::~CreatureDatabase()
{
    clear();
}

void CreatureDatabase::clear()
{
    creature_map.clear();
}

CreatureType* CreatureDatabase::operator[](const std::string& name)
{
    auto iter = creature_map.find(as_lower_str(name));
    if (iter != creature_map.end()) {
        return iter->second.get();
    }
    return nullptr;
}

CreatureType* CreatureDatabase::addMissingCreatureType(const std::string& name, bool isNpc)
{
    assert((*this)[name] == nullptr);

    auto ct = std::make_unique<CreatureType>();
    ct->name = name;
    ct->isNpc = isNpc;
    ct->missing = true;
    ct->outfit.lookType = 128;
    ct->outfit.lookHead = 78;
    ct->outfit.lookBody = 69;
    ct->outfit.lookLegs = 58;
    ct->outfit.lookFeet = 76;
    ct->outfit.lookAddon = 0;

    auto [it, inserted] = creature_map.emplace(as_lower_str(name), std::move(ct));
    return it->second.get();
}

CreatureType* CreatureDatabase::addCreatureType(const std::string& name, bool isNpc, const Outfit& outfit)
{
    if (CreatureType* ct = (*this)[name]) {
        return ct;
    }

    auto ct = std::make_unique<CreatureType>();
    ct->name = name;
    ct->isNpc = isNpc;
    ct->missing = false;
    ct->outfit = outfit;

    auto [it, inserted] = creature_map.emplace(as_lower_str(name), std::move(ct));
    return it->second.get();
}

bool CreatureDatabase::hasMissing() const
{
    return std::ranges::any_of(creature_map, [](const auto& pair) { return pair.second->missing; });
}

bool CreatureDatabase::loadFromXML(const FileName& filename, bool standard, wxString& error, std::vector<std::string>& warnings)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
    if (!result) {
        error = "Couldn't open file \"" + filename.GetFullName() + "\", invalid format?";
        return false;
    }

    pugi::xml_node node = doc.child("creatures");
    if (!node) {
        error = "Invalid file signature, this file is not a valid creatures file.";
        return false;
    }

    for (pugi::xml_node creatureNode = node.first_child(); creatureNode; creatureNode = creatureNode.next_sibling()) {
        if (as_lower_str(creatureNode.name()) != "creature") {
            continue;
        }

        CreatureType* creatureTypePtr = CreatureType::loadFromXML(creatureNode, warnings);
        if (creatureTypePtr) {
            std::unique_ptr<CreatureType> creatureType(creatureTypePtr);
            creatureType->standard = standard;
            if ((*this)[creatureType->name]) {
                warnings.push_back(
                    (wxString("Duplicate creature type name \"") + wxstr(creatureType->name) + "\"! Discarding...").ToStdString()
                );
            } else {
                creature_map[as_lower_str(creatureType->name)] = std::move(creatureType);
            }
        }
    }
    return true;
}

static void ensureCreatureBrush(CreatureType* creatureType)
{
    if (creatureType->brush) {
        return;
    }

    Tileset* tileSet = nullptr;
    if (creatureType->isNpc) {
        tileSet = g_materials.tilesets["NPCs"];
    } else {
        tileSet = g_materials.tilesets["Others"];
    }
    ASSERT(tileSet != nullptr);

    auto brush = std::make_unique<CreatureBrush>(creatureType);
    creatureType->brush = brush.get();
    g_brushes.addBrush(std::move(brush));
    creatureType->in_other_tileset = true;

    TilesetCategory* tileSetCategory = tileSet->getCategory(TILESET_CREATURE);
    tileSetCategory->brushlist.push_back(creatureType->brush);
}

bool CreatureDatabase::importXMLFromOT(const FileName& filename, wxString& error, std::vector<std::string>& warnings)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.GetFullPath().mb_str());
    if (!result) {
        error = "Couldn't open file \"" + filename.GetFullName() + "\", invalid format?";
        return false;
    }

    pugi::xml_node node = doc.child("monsters");
    if (node) {
        for (pugi::xml_node monsterNode = node.first_child(); monsterNode; monsterNode = monsterNode.next_sibling()) {
            if (as_lower_str(monsterNode.name()) != "monster") {
                continue;
            }

            pugi::xml_attribute attribute = monsterNode.attribute("file");
            if (!attribute) {
                continue;
            }

            FileName monsterFile(filename);
            monsterFile.SetFullName(wxString(attribute.as_string(), wxConvUTF8));

            pugi::xml_document monsterDoc;
            pugi::xml_parse_result monsterResult = monsterDoc.load_file(monsterFile.GetFullPath().mb_str());
            if (!monsterResult) {
                continue;
            }

            CreatureType* creatureTypeRaw = CreatureType::loadFromOTXML(monsterFile, monsterDoc, warnings);
            if (creatureTypeRaw) {
                std::unique_ptr<CreatureType> creatureType(creatureTypeRaw);
                CreatureType* current = (*this)[creatureType->name];
                if (current) {
                    CreatureType::preserve_assign_creature_fields(current, *creatureType);
                } else {
                    CreatureType* ptr = creatureType.get();
                    creature_map[as_lower_str(ptr->name)] = std::move(creatureType);

                    ensureCreatureBrush(ptr);
                }
            }
        }
    } else {
        node = doc.child("monster");
        if (!node) {
            node = doc.child("npc");
        }

        if (node) {
            CreatureType* creatureTypeRaw = CreatureType::loadFromOTXML(filename, doc, warnings);
            if (creatureTypeRaw) {
                std::unique_ptr<CreatureType> creatureType(creatureTypeRaw);
                CreatureType* current = (*this)[creatureType->name];

                if (current) {
                    CreatureType::preserve_assign_creature_fields(current, *creatureType);
                } else {
                    CreatureType* ptr = creatureType.get();
                    creature_map[as_lower_str(ptr->name)] = std::move(creatureType);

                    ensureCreatureBrush(ptr);
                }
            }
        }
    }
    return true;
}

bool CreatureDatabase::saveToXML(const FileName& filename)
{
    pugi::xml_document doc;

    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";

    pugi::xml_node creatureNodes = doc.append_child("creatures");
    for (const auto& [name, creatureType] : creature_map) {
        if (!creatureType->standard) {
            pugi::xml_node creatureNode = creatureNodes.append_child("creature");

            creatureNode.append_attribute("name") = creatureType->name.c_str();
            creatureNode.append_attribute("type") = creatureType->isNpc ? "npc" : "monster";

            const Outfit& outfit = creatureType->outfit;
            creatureNode.append_attribute("looktype") = outfit.lookType;
            creatureNode.append_attribute("lookitem") = outfit.lookItem;
            creatureNode.append_attribute("lookmount") = outfit.lookMount;
            creatureNode.append_attribute("lookaddon") = outfit.lookAddon;
            creatureNode.append_attribute("lookhead") = outfit.lookHead;
            creatureNode.append_attribute("lookbody") = outfit.lookBody;
            creatureNode.append_attribute("looklegs") = outfit.lookLegs;
            creatureNode.append_attribute("lookfeet") = outfit.lookFeet;
            creatureNode.append_attribute("lookmounthead") = outfit.lookMountHead;
            creatureNode.append_attribute("lookmountbody") = outfit.lookMountBody;
            creatureNode.append_attribute("lookmountlegs") = outfit.lookMountLegs;
            creatureNode.append_attribute("lookmountfeet") = outfit.lookMountFeet;
        }
    }
    return doc.save_file(filename.GetFullPath().mb_str(), "\t", pugi::format_default, pugi::encoding_utf8);
}
