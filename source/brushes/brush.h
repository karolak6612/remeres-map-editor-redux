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

/**
 * @file brush.h
 * @brief Base classes and management for the brush system.
 *
 * Defines the abstract Brush interface, the Brushes container, and specific
 * brush implementations like TerrainBrush and EraserBrush.
 */

#ifndef RME_BRUSH_H_
#define RME_BRUSH_H_
#include "app/main.h"

#include "map/position.h"

#include "brushes/brush_enums.h"
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include <string_view>

// Thanks to a million forward declarations, we don't have to include any files!
// TODO move to a declarations file.
class ItemType;
class Sprite;
class CreatureType;
class BaseMap;
class House;
class Item;
class Tile;
using TileVector = std::vector<Tile*>;
class AutoBorder;
class Brush;
class RAWBrush;
class DoodadBrush;
class TerrainBrush;
class GroundBrush;
class WallBrush;
class WallDecorationBrush;
class TableBrush;
class CarpetBrush;
class DoorBrush;
class OptionalBorderBrush;
class CreatureBrush;
class SpawnBrush;
class HouseBrush;
class HouseExitBrush;
class WaypointBrush;
class FlagBrush;
class EraserBrush;

//=============================================================================
// Brushes, holds all brushes

using BrushMap = std::multimap<std::string, std::unique_ptr<Brush>, std::less<>>;

/**
 * @brief Container class managing all available brushes.
 *
 * Handles loading, storing, and retrieving brushes by name or ID.
 * It also manages auto-border configurations.
 */
class Brushes {
public:
	Brushes();
	~Brushes();

	/**
	 * @brief Initializes the brush system.
	 */
	void init();

	/**
	 * @brief Clears all loaded brushes.
	 */
	void clear();

	/**
	 * @brief Retrieves a brush by its name.
	 * @param name The name of the brush.
	 * @return Pointer to the brush, or nullptr if not found.
	 */
	Brush* getBrush(std::string_view name) const;

	/**
	 * @brief Adds a new brush to the collection.
	 * @param brush The brush to add. Ownership is transferred to the Brushes object.
	 */
	void addBrush(Brush* brush);

	/**
	 * @brief Loads border configuration from XML.
	 * @param node The XML node.
	 * @param warnings Vector to store warning messages.
	 * @return true if successful.
	 */
	bool unserializeBorder(pugi::xml_node node, std::vector<std::string>& warnings);

	/**
	 * @brief Loads a brush definition from XML.
	 * @param node The XML node.
	 * @param warnings Vector to store warning messages.
	 * @return true if successful.
	 */
	bool unserializeBrush(pugi::xml_node node, std::vector<std::string>& warnings);

	/**
	 * @brief Gets the underlying map of all brushes.
	 * @return Reference to the BrushMap.
	 */
	const BrushMap& getMap() const {
		return brushes;
	}

protected:
	using BorderMap = std::unordered_map<uint32_t, std::unique_ptr<AutoBorder>>;
	BrushMap brushes;
	BorderMap borders;

	friend class AutoBorder;
	friend class GroundBrush;
	friend class GroundBrushLoader;
};

extern Brushes g_brushes;

//=============================================================================
// Common brush interface

/**
 * @brief Abstract base class for all map editor brushes.
 *
 * A Brush defines how user input is translated into map modifications.
 * It supports drawing, undrawing, and validation checks.
 */
class Brush {
public:
	Brush();
	virtual ~Brush();

	/**
	 * @brief Loads brush data from an XML node.
	 * @param node The XML configuration.
	 * @param warnings Output warnings.
	 * @return true on success.
	 */
	virtual bool load(pugi::xml_node node, std::vector<std::string>& warnings) {
		return true;
	}

	/**
	 * @brief Applies the brush to a specific tile.
	 * @param map The map being edited.
	 * @param tile The target tile.
	 * @param parameter Optional extra data (e.g., alt-key state).
	 */
	virtual void draw(BaseMap* map, Tile* tile, void* parameter = nullptr) = 0;

	/**
	 * @brief Removes the brush's effect from a tile (erasing).
	 * @param map The map being edited.
	 * @param tile The target tile.
	 */
	virtual void undraw(BaseMap* map, Tile* tile) = 0;

	/**
	 * @brief Checks if the brush can be applied at the given position.
	 * @param map The map context.
	 * @param position The target coordinates.
	 * @return true if drawing is valid.
	 */
	virtual bool canDraw(BaseMap* map, const Position& position) const = 0;

	//
	/**
	 * @brief Gets the unique ID of this brush instance.
	 * @return The brush ID.
	 */
	uint32_t getID() const {
		return id;
	}

	/**
	 * @brief Gets the name of the brush.
	 * @return The name string.
	 */
	virtual std::string getName() const = 0;

	/**
	 * @brief Sets the name of the brush.
	 * @param newName The new name.
	 */
	virtual void setName(const std::string& newName) {
		ASSERT(_MSG("setName attempted on nameless brush!"));
	}

	/**
	 * @brief Gets the look ID (often item ID) associated with the brush.
	 * @return The look ID.
	 */
	virtual int getLookID() const = 0;

	/**
	 * @brief Gets the sprite used for rendering the brush cursor.
	 * @return Pointer to the Sprite object.
	 */
	virtual Sprite* getSprite() const {
		return nullptr;
	}

	/**
	 * @brief Checks if the brush triggers auto-border logic.
	 * @return true if borders are needed.
	 */
	virtual bool needBorders() const {
		return false;
	}

	/**
	 * @brief Checks if the brush supports drag-and-drop operations.
	 * @return true if draggable.
	 */
	virtual bool canDrag() const {
		return false;
	}

	/**
	 * @brief Checks if the brush can be "smeared" (drawn continuously while dragging mouse).
	 * @return true if smearable.
	 */
	virtual bool canSmear() const {
		return true;
	}

	/**
	 * @brief Checks if the brush applies to all sizes equally.
	 * @return true if one size fits all.
	 */
	virtual bool oneSizeFitsAll() const {
		return false;
	}

	/**
	 * @brief Gets the maximum variation index supported by the brush.
	 * @return Max variation count.
	 */
	virtual int32_t getMaxVariation() const {
		return 0;
	}

	/**
	 * @brief Dynamic cast helper to convert to a specific brush type.
	 * @tparam T Target type.
	 * @return Pointer to T or nullptr.
	 */
	template <typename T>
	T* as() {
		return dynamic_cast<T*>(this);
	}

	/**
	 * @brief Dynamic cast helper (const version).
	 * @tparam T Target type.
	 * @return Const pointer to T or nullptr.
	 */
	template <typename T>
	const T* as() const {
		return dynamic_cast<const T*>(this);
	}

	/**
	 * @brief Type check helper.
	 * @tparam T Type to check against.
	 * @return true if this brush is of type T.
	 */
	template <typename T>
	bool is() const {
		return dynamic_cast<const T*>(this) != nullptr;
	}

	/**
	 * @brief Checks if the brush should appear in the palette UI.
	 * @return true if visible.
	 */
	bool visibleInPalette() const {
		return visible;
	}

	/**
	 * @brief Flags the brush as visible in the palette.
	 */
	void flagAsVisible() {
		visible = true;
	}

	/**
	 * @brief Checks if the brush uses the collection system.
	 * @return true if it uses collections.
	 */
	bool hasCollection() const {
		return usesCollection;
	}

	/**
	 * @brief Marks the brush as using the collection system.
	 */
	void setCollection() {
		usesCollection = true;
	}

	/**
	 * @brief Gets list of items related to this brush (e.g. for palette display).
	 * @param items Vector to populate with item IDs.
	 */
	virtual void getRelatedItems(std::vector<uint16_t>& items) { }

protected:
	static uint32_t id_counter;
	uint32_t id;
	bool visible; // Visible in any palette?
	bool usesCollection;
};

//=============================================================================
// Terrain brush interface

/**
 * @brief Base class for brushes that paint terrain (ground, etc.).
 *
 * Handles common logic for terrain brushes, including friendship/hate logic
 * for auto-bordering.
 */
class TerrainBrush : public Brush {
public:
	TerrainBrush();
	~TerrainBrush() override;

	bool canDraw(BaseMap* map, const Position& position) const override {
		return true;
	}

	std::string getName() const override {
		return name;
	}
	void setName(const std::string& newName) override {
		name = newName;
	}

	virtual int32_t getZ() const {
		return 0;
	}
	int32_t getLookID() const override {
		return look_id;
	}

	bool needBorders() const override {
		return true;
	}
	bool canDrag() const override {
		return true;
	}

	/**
	 * @brief Checks if this brush is "friends" with another brush (for bordering).
	 * @param other The other brush.
	 * @return true if they are friends.
	 */
	bool friendOf(TerrainBrush* other);

protected:
	std::vector<uint32_t> friends;
	std::string name;
	uint16_t look_id;
	bool hate_friends;
};

//=============================================================================
// Rampbrush, add ramps to mountains.

/**
 * @brief Special brush for erasing items from the map.
 *
 * Removes items on the target tile.
 */
class EraserBrush : public Brush {
public:
	EraserBrush();
	~EraserBrush() override;

	bool canDraw(BaseMap* map, const Position& position) const override;
	void draw(BaseMap* map, Tile* tile, void* parameter) override;
	void undraw(BaseMap* map, Tile* tile) override;

	bool needBorders() const override {
		return true;
	}
	bool canDrag() const override {
		return true;
	}
	int getLookID() const override;
	std::string getName() const override;
};

//=============================================================================

#endif
