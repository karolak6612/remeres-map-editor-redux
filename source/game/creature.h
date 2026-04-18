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

#ifndef RME_CREATURE_H_
#define RME_CREATURE_H_

#include "game/creatures.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

enum Direction {
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	DIRECTION_FIRST = NORTH,
	DIRECTION_LAST = WEST
};

IMPLEMENT_INCREMENT_OP(Direction)

class Creature {
public:
	Creature(CreatureType* ctype);
	Creature(std::string type_name);
	~Creature();

	// Static conversions
	static std::string DirID2Name(uint16_t id);
	static uint16_t DirName2ID(std::string id);

	std::unique_ptr<Creature> deepCopy() const;

	const Outfit& getLookType() const;

	bool isSaved();
	void save();
	void reset();

	bool isSelected() const {
		return selected;
	}
	void deselect() {
		selected = false;
	}
	void select() {
		selected = true;
	}

	bool isNpc() const;

	std::string getName() const;
	CreatureBrush* getBrush() const;

	int getSpawnTime() const {
		return spawntime;
	}
	void setSpawnTime(int spawntime) {
		this->spawntime = spawntime;
	}

	Direction getDirection() const {
		return direction;
	}
	void setDirection(Direction direction) {
		this->direction = direction;
	}

	[[nodiscard]] std::optional<uint8_t> getSpawnWeight() const {
		return spawn_weight;
	}
	void setSpawnWeight(std::optional<uint8_t> weight) {
		spawn_weight = weight;
	}
	void clearSpawnWeight() {
		spawn_weight.reset();
	}

	[[nodiscard]] const std::map<std::string, std::string>& getSpawnAttributes() const {
		return spawn_attributes;
	}
	void setSpawnAttribute(std::string key, std::string value) {
		spawn_attributes[std::move(key)] = std::move(value);
	}
	void setSpawnAttributes(std::map<std::string, std::string> attributes) {
		spawn_attributes = std::move(attributes);
	}
	void clearSpawnAttributes() {
		spawn_attributes.clear();
	}

protected:
	std::string type_name;
	Direction direction;
	int spawntime;
	std::optional<uint8_t> spawn_weight;
	std::map<std::string, std::string> spawn_attributes;
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
