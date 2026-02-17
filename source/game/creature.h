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

/*
 * @file creature.h
 * @brief Represents a creature or NPC in the game world.
 *
 * Defines the Creature class and related enumerations for handling
 * player-facing entities, monsters, and NPCs within the editor.
 */

#ifndef RME_CREATURE_H_
#define RME_CREATURE_H_

#include "game/creatures.h"

/*
 * @brief Cardinal directions for creature facing.
 */
enum Direction {
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	DIRECTION_FIRST = NORTH,
	DIRECTION_LAST = WEST
};

IMPLEMENT_INCREMENT_OP(Direction)

/*
 * @brief Represents an instance of a creature placed on the map.
 *
 * A Creature holds instance-specific data like spawn time, direction, and name,
 * while referencing a shared CreatureType definition for common properties.
 */
class Creature {
public:
	/*
	 * @brief Constructs a creature from a type definition.
	 * @param ctype The creature type definition (e.g., "Dragon").
	 */
	Creature(CreatureType* ctype);

	/*
	 * @brief Constructs a creature by type name.
	 * @param type_name The name of the creature type.
	 */
	Creature(std::string type_name);
	~Creature();

	// Static conversions
	/*
	 * @brief Converts a direction ID to its string representation.
	 * @param id The direction ID (0-3).
	 * @return The direction name (e.g., "North").
	 */
	static std::string DirID2Name(uint16_t id);

	/*
	 * @brief Converts a direction name string to its ID.
	 * @param id The direction name.
	 * @return The direction ID.
	 */
	static uint16_t DirName2ID(std::string id);

	/*
	 * @brief Creates a deep copy of this creature instance.
	 * @return Unique pointer to the new creature.
	 */
	std::unique_ptr<Creature> deepCopy() const;

	/*
	 * @brief Gets the visual outfit of the creature.
	 * @return Reference to the Outfit.
	 */
	const Outfit& getLookType() const;

	/*
	 * @brief Checks if the creature has been saved to disk.
	 * @return true if saved.
	 */
	bool isSaved();

	/*
	 * @brief Marks the creature as saved.
	 */
	void save();

	/*
	 * @brief Resets the saved status (marks as unsaved).
	 */
	void reset();

	/*
	 * @brief Checks if the creature is currently selected in the editor.
	 * @return true if selected.
	 */
	bool isSelected() const {
		return selected;
	}

	/*
	 * @brief Deselects the creature.
	 */
	void deselect() {
		selected = false;
	}

	/*
	 * @brief Selects the creature.
	 */
	void select() {
		selected = true;
	}

	/*
	 * @brief Checks if the creature is an NPC.
	 * @return true if NPC.
	 */
	bool isNpc() const;

	/*
	 * @brief Gets the name of the creature.
	 * @return The name string.
	 */
	std::string getName() const;

	/*
	 * @brief Gets the editor brush associated with this creature type.
	 * @return Pointer to CreatureBrush.
	 */
	CreatureBrush* getBrush() const;

	/*
	 * @brief Gets the spawn time interval for this creature instance.
	 * @return Spawn time in seconds.
	 */
	int getSpawnTime() const {
		return spawntime;
	}

	/*
	 * @brief Sets the spawn time interval.
	 * @param spawntime Time in seconds.
	 */
	void setSpawnTime(int spawntime) {
		this->spawntime = spawntime;
	}

	/*
	 * @brief Gets the facing direction of the creature.
	 * @return The Direction enum.
	 */
	Direction getDirection() const {
		return direction;
	}

	/*
	 * @brief Sets the facing direction.
	 * @param direction The new direction.
	 */
	void setDirection(Direction direction) {
		this->direction = direction;
	}

protected:
	std::string type_name;
	Direction direction;
	int spawntime;
	bool saved;
	bool selected;
};

inline void Creature::save() {
	saved = true;
}

inline void Creature::reset() {
	saved = false;
}

inline bool Creature::isSaved() {
	return saved;
}

inline bool Creature::isNpc() const {
	CreatureType* type = g_creatures[type_name];
	if (type) {
		return type->isNpc;
	}
	return false;
}

inline std::string Creature::getName() const {
	CreatureType* type = g_creatures[type_name];
	if (type) {
		return type->name;
	}
	return "";
}
inline CreatureBrush* Creature::getBrush() const {
	CreatureType* type = g_creatures[type_name];
	if (type) {
		return type->brush;
	}
	return nullptr;
}

using CreatureVector = std::vector<Creature*>;
using CreatureList = std::list<Creature*>;

#endif
