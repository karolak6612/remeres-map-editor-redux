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
#include "lua_api_app.h"
#include "lua_script_manager.h"
#include "ui/gui.h"
#include "editor/editor.h"
#include "editor/action_queue.h"
#include "map/map.h"
#include "brushes/brush.h"
#include "editor/action.h"
#include "map/tile.h"
#include "editor/selection.h"
#include "game/items.h"
#include "brushes/raw/raw_brush.h"
#include "brushes/ground/auto_border.h"
#include "brushes/ground/ground_brush.h"
#include "brushes/wall/wall_brush.h"
#include "util/file_system.h"
#include "ext/pugixml.hpp"

#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <cstdio>
#include <thread>
#include <chrono>

namespace LuaAPI {

// ============================================================================
// Transaction System for Undo/Redo Support
// ============================================================================

// Transaction implementation
#include "game/house.h"

	// Helper to sync map metadata when swapping tiles
	static void updateTileMetadata(Editor* editor, Tile* tile, bool adding) {
		if (!editor || !tile) {
			return;
		}
		Map* map = editor->getMap();
		if (!map) {
			return;
		}

		if (adding) {
			if (tile->spawn) {
				map->addSpawn(tile);
			}
			if (tile->getHouseID()) {
				House* h = map->houses.getHouse(tile->getHouseID());
				if (h) {
					h->addTile(tile);
				}
			}
		} else {
			if (tile->spawn) {
				map->removeSpawn(tile);
			}
			if (tile->getHouseID()) {
				House* h = map->houses.getHouse(tile->getHouseID());
				if (h) {
					h->removeTile(tile);
				}
			}
			// Also clean up selection if removing
			if (tile->isSelected()) {
				// We use internal session to avoid creating undo actions for this cleanup
				editor->selection.start(Selection::INTERNAL);
				editor->selection.removeInternal(tile);
				editor->selection.finish(Selection::INTERNAL);
			}
		}
	}

	class LuaTransaction {
		bool active;
		Editor* editor;
		std::unique_ptr<BatchAction> batch;
		std::unique_ptr<Action> action;
		std::unordered_map<uint64_t, std::unique_ptr<Tile>> originalTiles;

		uint64_t positionKey(const Position& pos) const {
			return (static_cast<uint64_t>(pos.x) << 32) | (static_cast<uint64_t>(pos.y) << 16) | static_cast<uint64_t>(pos.z);
		}

		Position positionFromKey(uint64_t key) const {
			Position pos;
			pos.x = static_cast<int>(static_cast<int32_t>(key >> 32));
			pos.y = static_cast<int>(static_cast<uint16_t>((key >> 16) & 0xFFFFu));
			pos.z = static_cast<int>(static_cast<uint16_t>(key & 0xFFFFu));
			return pos;
		}

	public:
		static LuaTransaction& getInstance() {
			static LuaTransaction instance;
			return instance;
		}

		LuaTransaction() :
			active(false), editor(nullptr), batch(nullptr), action(nullptr) { }

		void begin(Editor* ed, const std::string& name = "") {
			if (active) {
				throw sol::error("Transaction already in progress");
			}
 
			editor = ed;
			if (!editor || !editor->actionQueue) {
				throw sol::error("No editor or action queue available");
			}
 
			active = true;
			batch = editor->actionQueue->createBatch(ACTION_LUA_SCRIPT);
			if (!name.empty()) {
				batch->setLabel(name);
			}
			action = editor->actionQueue->createAction(ACTION_LUA_SCRIPT);
			originalTiles.clear();
		}

		void commit() {
			if (!active) {
				return;
			}

			// Process each modified tile
			for (auto& pair : originalTiles) {
				const uint64_t key = pair.first;
				std::unique_ptr<Tile>& originalTile = pair.second;
				Position pos = positionFromKey(key);

				// Swap the original back into the map, or remove the tile entirely if it did not exist before
				std::unique_ptr<Tile> modifiedTile = editor->getMap()->swapTile(
					pos,
					originalTile ? std::move(originalTile) : std::unique_ptr<Tile>()
				);

				// Swapped-out state should be cleaned up from metadata and selection
				if (modifiedTile) {
					updateTileMetadata(editor, modifiedTile.get(), false);
				}

				// Add the current map tile back to metadata if one now exists
				Tile* tileInMap = editor->getMap()->getTile(pos);
				if (tileInMap) {
					updateTileMetadata(editor, tileInMap, true);
				}

				// Create Change with the actual modified tile object so pointer identity is preserved
				action->addChange(std::make_unique<Change>(std::move(modifiedTile), pos));
			}

			// Clear - ownership has been transferred or tiles discarded
			originalTiles.clear();

			if (action->size() > 0) {
				batch->addAndCommitAction(std::move(action));
				editor->addBatch(std::move(batch));
				editor->getMap()->doChange();
				g_gui.RefreshView(); // Force redraw immediately
			} else {
				// No changes, clean up
				action.reset();
				batch.reset();
			}

			cleanup();
		}

		void rollback() {
			if (!active) {
				return;
			}

			// Restore original tiles (discard any changes made)
			for (auto& pair : originalTiles) {
				const uint64_t key = pair.first;
				std::unique_ptr<Tile>& originalTile = pair.second;
				Position pos = positionFromKey(key);

				std::unique_ptr<Tile> modifiedTile = originalTile ? editor->getMap()->swapTile(pos, std::move(originalTile)) : editor->getMap()->swapTile(pos, std::unique_ptr<Tile>());

				// Clean up modified tile
				if (modifiedTile) {
					updateTileMetadata(editor, modifiedTile.get(), false);
				}

				// Restore original tile metadata if one exists
				Tile* tileInMap = editor->getMap()->getTile(pos);
				if (tileInMap) {
					updateTileMetadata(editor, tileInMap, true);
				}
			}
			originalTiles.clear();

			// Discard without committing
			action.reset();
			batch.reset();

			cleanup();
		}

		void markTileModified(Tile* tile, bool originallyExisted) {
			if (!active || !tile) {
				return;
			}

			Position pos = tile->getPosition();
			uint64_t key = positionKey(pos);

			// Only snapshot the tile once per transaction (first time it's modified)
			if (originalTiles.find(key) == originalTiles.end()) {
				// Preserve whether the tile originally existed so delete/create can undo cleanly
				originalTiles[key] = originallyExisted ? tile->deepCopy() : std::unique_ptr<Tile>();
			}
		}

		bool isActive() const {
			return active;
		}
		Editor* getEditor() const {
			return editor;
		}

	private:
		void cleanup() {
			active = false;
			editor = nullptr;
			batch.reset();
			action.reset();
		}
	};

	// Global accessor for tile modification tracking (used by lua_api_tile.cpp)
	void markTileForUndo(Tile* tile, bool originallyExisted) {
		if (LuaTransaction::getInstance().isActive()) {
			LuaTransaction::getInstance().markTileModified(tile, originallyExisted);
		}
	}

	// ============================================================================
	// Helper Functions
	// ============================================================================

	// Helper function to show alert dialog
	static int showAlert(sol::this_state ts, sol::object arg) {
		sol::state_view lua(ts);

		std::string title = "Script";
		std::string message;
		std::vector<std::string> buttons;

		// Handle different argument types
		if (arg.is<std::string>()) {
			message = arg.as<std::string>();
			buttons.push_back("OK");
		} else if (arg.is<sol::table>()) {
			sol::table opts = arg.as<sol::table>();

			if (opts["title"].valid()) {
				title = opts["title"].get<std::string>();
			}
			if (opts["text"].valid()) {
				message = opts["text"].get<std::string>();
			}
			if (opts["buttons"].valid()) {
				sol::table btns = opts["buttons"];
				for (size_t i = 1; i <= btns.size(); ++i) {
					if (btns[i].valid()) {
						buttons.push_back(btns[i].get<std::string>());
					}
				}
			}

			if (buttons.empty()) {
				buttons.push_back("OK");
			}
		} else {
			sol::function tostring = lua["tostring"];
			message = tostring(arg);
			buttons.push_back("OK");
		}

		// Determine dialog style based on buttons
		long style = wxCENTRE;
		if (buttons.size() == 1) {
			style |= wxOK;
		} else if (buttons.size() == 2) {
			std::string btn1 = buttons[0];
			std::string btn2 = buttons[1];
			std::transform(btn1.begin(), btn1.end(), btn1.begin(), ::tolower);
			std::transform(btn2.begin(), btn2.end(), btn2.begin(), ::tolower);

			if ((btn1 == "ok" && btn2 == "cancel") || (btn1 == "cancel" && btn2 == "ok")) {
				style |= wxOK | wxCANCEL;
			} else if ((btn1 == "yes" && btn2 == "no") || (btn1 == "no" && btn2 == "yes")) {
				style |= wxYES_NO;
			} else {
				style |= wxOK | wxCANCEL;
			}
		} else if (buttons.size() >= 3) {
			style |= wxYES_NO | wxCANCEL;
		}

		wxWindow* parent = g_gui.root;
		wxMessageDialog dlg(parent, wxString(message), wxString(title), style);

		int result = dlg.ShowModal();

		switch (result) {
			case wxID_OK:
			case wxID_YES:
				return 1;
			case wxID_NO:
				return 2;
			case wxID_CANCEL:
				return buttons.size() >= 3 ? 3 : 2;
			default:
				return 0;
		}
	}

	static sol::object chooseFile(sol::this_state ts, sol::object arg) {
		sol::state_view lua(ts);

		std::string title = "Select file";
		std::string path;
		std::string wildcard = "XML files (*.xml)|*.xml|All files (*.*)|*.*";

		if (arg.is<sol::table>()) {
			sol::table options = arg.as<sol::table>();
			title = options.get_or(std::string("title"), title);
			path = options.get_or(std::string("path"), std::string(""));
			wildcard = options.get_or(std::string("wildcard"), wildcard);
		} else if (arg.is<std::string>()) {
			path = arg.as<std::string>();
		}

		wxFileName fileName{wxString(path)};
		wxString defaultDir = fileName.GetPath();
		wxString defaultFile = fileName.GetFullName();
		if (defaultDir.IsEmpty()) {
			defaultDir = wxString(FileSystem::GetDataDirectory().ToStdString());
		}

		wxWindow* parent = g_gui.root;
		wxFileDialog dlg(
			parent,
			wxString(title),
			defaultDir,
			defaultFile,
			wxString(wildcard),
			wxFD_OPEN | wxFD_FILE_MUST_EXIST
		);

		if (dlg.ShowModal() != wxID_OK) {
			return sol::make_object(lua, sol::nil);
		}

		return sol::make_object(lua, dlg.GetPath().ToStdString());
	}

	// Check if a map is currently open
	static bool hasMap() {
		Editor* editor = g_gui.GetCurrentEditor();
		return editor != nullptr && editor->getMap() != nullptr;
	}

	// Refresh the map view
	static void refresh() {
		g_gui.RefreshView();
	}

	// Get the current Map object
	static Map* getMap() {
		Editor* editor = g_gui.GetCurrentEditor();
		if (!editor) {
			return nullptr;
		}
		return editor->getMap();
	}

	// Get the current Selection object
	static Selection* getSelection() {
		Editor* editor = g_gui.GetCurrentEditor();
		if (!editor) {
			return nullptr;
		}
		return &editor->selection;
	}

	static void setClipboard(const std::string& text) {
		if (wxTheClipboard->Open()) {
			wxTheClipboard->SetData(new wxTextDataObject(text));
			wxTheClipboard->Close();
		}
	}

	// Transaction function with undo/redo support
	static void transaction(const std::string& name, sol::function func) {
		Editor* editor = g_gui.GetCurrentEditor();
		if (!editor) {
			throw sol::error("No map open");
		}

		LuaTransaction& trans = LuaTransaction::getInstance();

		try {
			trans.begin(editor, name);
			func();
			if (g_gui.GetCurrentEditor() == editor) {
				trans.commit();
			} else {
				trans.rollback();
				throw sol::error("Editor changed during transaction");
			}
			g_gui.RefreshView();
		} catch (const sol::error&) {
			trans.rollback();
			throw; // Re-throw to show error to user
		} catch (const std::exception& e) {
			trans.rollback();
			throw sol::error(std::string("Transaction failed: ") + e.what());
		} catch (...) {
			trans.rollback();
			throw sol::error("Transaction failed with unknown error");
		}
	}

	static sol::object getBorders(sol::this_state ts) {
		sol::state_view lua(ts);

		sol::table bordersTable = lua.create_table();

		for (auto& pair : g_brushes.getBorders()) {
			AutoBorder* border = pair.second.get();
			if (!border) {
				continue;
			}

			sol::table b = lua.create_table();
			b["id"] = border->id;
			b["group"] = border->group;
			// b["ground"] = border->ground; // Check if ground exists in AutoBorder

			sol::table tiles = lua.create_table();
			for (int i = 0; i < 13; ++i) {
				tiles[i + 1] = border->tiles[i];
			}
			b["tiles"] = tiles;

			bordersTable[border->id] = b;
		}

		return bordersTable;
	}

	static sol::object getGrounds(sol::this_state ts) {
		sol::state_view lua(ts);

		sol::table groundsTable = lua.create_table();

		for (const auto& pair : g_brushes.getMap()) {
			const Brush* brush = pair.second.get();
			const GroundBrush* ground = brush ? brush->as<GroundBrush>() : nullptr;
			if (!ground) {
				continue;
			}

			sol::table g = lua.create_table();
			g["id"] = ground->getID();
			g["name"] = ground->getName();
			g["lookid"] = ground->getLookID();
			g["server_lookid"] = 0;
			g["z_order"] = ground->getZ();
			g["randomize"] = ground->isReRandomizable();
			g["solo_optional"] = ground->useSoloOptionalBorder();

			std::vector<std::pair<uint16_t, int>> itemEntries;
			ground->getItemChanceEntries(itemEntries);

			sol::table items = lua.create_table();
			for (size_t i = 0; i < itemEntries.size(); ++i) {
				sol::table item = lua.create_table();
				item["id"] = itemEntries[i].first;
				item["chance"] = itemEntries[i].second;
				items[static_cast<int>(i + 1)] = item;
			}
			g["items"] = items;

			if (!itemEntries.empty()) {
				sol::table mainItem = lua.create_table();
				mainItem["id"] = itemEntries.front().first;
				mainItem["chance"] = itemEntries.front().second;
				g["main_item"] = mainItem;
			} else {
				g["main_item"] = sol::nil;
			}

			groundsTable[ground->getName()] = g;
		}

		return groundsTable;
	}

	static std::string doorTypeToString(DoorType type) {
		switch (type) {
			case WALL_ARCHWAY:
				return "archway";
			case WALL_DOOR_NORMAL:
				return "normal";
			case WALL_DOOR_LOCKED:
				return "locked";
			case WALL_DOOR_QUEST:
				return "quest";
			case WALL_DOOR_MAGIC:
				return "magic";
			case WALL_DOOR_NORMAL_ALT:
				return "normal_alt";
			case WALL_WINDOW:
				return "window";
			case WALL_HATCH_WINDOW:
				return "hatch_window";
			default:
				return "undefined";
		}
	}

	static sol::object chooseFile(sol::this_state ts, sol::object arg) {
		sol::state_view lua(ts);

		std::string title = "Select file";
		std::string path;
		std::string wildcard = "XML files (*.xml)|*.xml|All files (*.*)|*.*";

		if (arg.is<sol::table>()) {
			sol::table options = arg.as<sol::table>();
			title = options.get_or(std::string("title"), title);
			path = options.get_or(std::string("path"), std::string(""));
			wildcard = options.get_or(std::string("wildcard"), wildcard);
		} else if (arg.is<std::string>()) {
			path = arg.as<std::string>();
		}

		wxFileName fileName{wxString(path)};
		wxString defaultDir = fileName.GetPath();
		wxString defaultFile = fileName.GetFullName();
		if (defaultDir.IsEmpty()) {
			defaultDir = wxString(FileSystem::GetDataDirectory().ToStdString());
		}

		wxWindow* parent = g_gui.root;
		wxFileDialog dlg(
			parent,
			wxString(title),
			defaultDir,
			defaultFile,
			wxString(wildcard),
			wxFD_OPEN | wxFD_FILE_MUST_EXIST
		);

		if (dlg.ShowModal() != wxID_OK) {
			return sol::make_object(lua, sol::nil);
		}

		return sol::make_object(lua, dlg.GetPath().ToStdString());
	}

	static std::string wallAlignmentToString(int alignment) {
		switch (alignment) {
			case WALL_VERTICAL:
				return "vertical";
			case WALL_HORIZONTAL:
				return "horizontal";
			case WALL_NORTHWEST_DIAGONAL:
				return "corner";
			case WALL_POLE:
				return "pole";
			case WALL_SOUTH_END:
				return "south end";
			case WALL_EAST_END:
				return "east end";
			case WALL_NORTH_END:
				return "north end";
			case WALL_WEST_END:
				return "west end";
			case WALL_SOUTH_T:
				return "south T";
			case WALL_EAST_T:
				return "east T";
			case WALL_WEST_T:
				return "west T";
			case WALL_NORTH_T:
				return "north T";
			case WALL_NORTHEAST_DIAGONAL:
				return "northeast diagonal";
			case WALL_SOUTHWEST_DIAGONAL:
				return "southwest diagonal";
			case WALL_SOUTHEAST_DIAGONAL:
				return "southeast diagonal";
			case WALL_INTERSECTION:
				return "intersection";
			case WALL_UNTOUCHABLE:
				return "untouchable";
			default:
				return "unknown";
		}
	}

	static sol::object getWalls(sol::this_state ts) {
		sol::state_view lua(ts);

		sol::table wallsTable = lua.create_table();

		for (const auto& pair : g_brushes.getMap()) {
			const Brush* brush = pair.second.get();
			const WallBrush* wall = brush ? brush->as<WallBrush>() : nullptr;
			if (!wall || wall->is<WallDecorationBrush>()) {
				continue;
			}

			sol::table wallTable = lua.create_table();
			wallTable["id"] = wall->getID();
			wallTable["name"] = wall->getName();
			wallTable["lookid"] = wall->getLookID();
			wallTable["kind"] = "wall";
			uint16_t serverLookId = 0;

			sol::table alignments = lua.create_table();
			for (int alignment = 0; alignment < WallBrushItems::WALL_ALIGNMENT_COUNT; ++alignment) {
				sol::table bucket = lua.create_table();
				bucket["token"] = wallAlignmentToString(alignment);

				sol::table items = lua.create_table();
				const auto& wallNode = wall->items.getWallNode(alignment);
				for (size_t itemIndex = 0; itemIndex < wallNode.items.size(); ++itemIndex) {
					sol::table item = lua.create_table();
					item["id"] = wallNode.items[itemIndex].id;
					int previousChance = itemIndex == 0 ? 0 : wallNode.items[itemIndex - 1].chance;
					item["chance"] = wallNode.items[itemIndex].chance - previousChance;
					items[static_cast<int>(itemIndex + 1)] = item;
				}
				bucket["items"] = items;

				sol::table doors = lua.create_table();
				const auto& doorItems = wall->items.getDoorItems(alignment);
				for (size_t doorIndex = 0; doorIndex < doorItems.size(); ++doorIndex) {
					const auto& doorItem = doorItems[doorIndex];
					const auto definition = g_item_definitions.get(doorItem.id);

					sol::table door = lua.create_table();
					door["id"] = doorItem.id;
					door["type"] = doorTypeToString(doorItem.type);
					door["locked"] = doorItem.locked;
					door["open"] = definition.hasFlag(ItemFlag::IsOpen);
					door["hate"] = definition.hasFlag(ItemFlag::WallHateMe);
					doors[static_cast<int>(doorIndex + 1)] = door;
				}
				bucket["doors"] = doors;
				if (serverLookId == 0) {
					if (wallNode.items.size() > 0) {
						serverLookId = wallNode.items.front().id;
					} else if (doorItems.size() > 0) {
						serverLookId = doorItems.front().id;
					}
				}

				alignments[wallAlignmentToString(alignment)] = bucket;
			}
			wallTable["alignments"] = alignments;
			wallTable["server_lookid"] = serverLookId;

			sol::table friends = lua.create_table();
			int friendIndex = 1;
			for (uint32_t friendId : wall->getFriendIds()) {
				for (const auto& candidatePair : g_brushes.getMap()) {
					const Brush* candidate = candidatePair.second.get();
					if (candidate && candidate->getID() == friendId) {
						friends[friendIndex++] = candidate->getName();
						break;
					}
				}
			}
			wallTable["friends"] = friends;
			wallTable["redirect_to"] = wall->getRedirectTo() ? wall->getRedirectTo()->getName() : "";

			wallsTable[wall->getName()] = wallTable;
		}

		return wallsTable;
	}

	static std::string getDataDirectory() {
		return FileSystem::GetDataDirectory().ToStdString();
	}

	static std::vector<std::string> splitPath(std::string_view path) {
		std::vector<std::string> parts;
		size_t start = 0;
		while (start < path.size()) {
			size_t slash = path.find('/', start);
			size_t count = slash == std::string_view::npos ? path.size() - start : slash - start;
			if (count > 0) {
				parts.emplace_back(path.substr(start, count));
			}
			if (slash == std::string_view::npos) {
				break;
			}
			start = slash + 1;
		}
		return parts;
	}

	static pugi::xml_node findNodeByPath(pugi::xml_node root, const std::string& path) {
		if (!root) {
			return pugi::xml_node();
		}

		if (path.empty()) {
			return root;
		}

		pugi::xml_node current = root;
		const auto parts = splitPath(path);
		for (size_t index = 0; index < parts.size(); ++index) {
			const std::string& part = parts[index];
			if (index == 0 && current.type() == pugi::node_element && current.name() == part) {
				continue;
			}
			current = current.child(part.c_str());
			if (!current) {
				return pugi::xml_node();
			}
		}

		return current;
	}

	static std::tuple<bool, std::string> upsertXmlNodeFile(const std::string& path, const sol::table& options) {
		const std::string rootName = options.get_or(std::string("root"), std::string("root"));
		const std::string parentPath = options.get_or(std::string("parent"), rootName);
		const std::string fragment = options.get_or(std::string("xml"), std::string(""));
		const std::string matchTag = options.get_or(std::string("tag"), std::string(""));
		const std::string matchAttr = options.get_or(std::string("match_attr"), std::string(""));
		const std::string matchValue = options.get_or(std::string("match_value"), std::string(""));

		if (fragment.empty()) {
			return { false, "Missing XML fragment." };
		}

		pugi::xml_document document;
		bool fileExists = std::filesystem::exists(path);
		if (fileExists) {
			const pugi::xml_parse_result loaded = document.load_file(path.c_str(), pugi::parse_default);
			if (!loaded) {
				return { false, "Failed to parse XML file: " + std::string(loaded.description()) };
			}
		}

		pugi::xml_node root = document.document_element();
		if (!root) {
			document.reset();
			document.append_child(pugi::node_declaration).append_attribute("version") = "1.0";
			document.first_child().append_attribute("encoding") = "utf-8";
			root = document.append_child(rootName.c_str());
		}

		if (!root || std::string(root.name()) != rootName) {
			return { false, "Root node does not match expected <" + rootName + ">." };
		}

		pugi::xml_node parent = findNodeByPath(root, parentPath);
		if (!parent) {
			return { false, "Parent node path not found: " + parentPath };
		}

		pugi::xml_document fragmentDocument;
		const pugi::xml_parse_result fragmentLoaded = fragmentDocument.load_buffer(fragment.data(), fragment.size(), pugi::parse_default);
		if (!fragmentLoaded) {
			return { false, "Failed to parse XML fragment: " + std::string(fragmentLoaded.description()) };
		}

		pugi::xml_node fragmentNode = fragmentDocument.document_element();
		if (!fragmentNode) {
			return { false, "XML fragment does not contain an element node." };
		}

		pugi::xml_node existing;
		if (!matchTag.empty() && !matchAttr.empty()) {
			for (pugi::xml_node child = parent.child(matchTag.c_str()); child; child = child.next_sibling(matchTag.c_str())) {
				if (child.attribute(matchAttr.c_str()).as_string() == matchValue) {
					existing = child;
					break;
				}
			}
		}

		if (existing) {
			parent.insert_copy_before(fragmentNode, existing);
			parent.remove_child(existing);
		} else {
			parent.append_copy(fragmentNode);
		}

		if (!document.save_file(path.c_str(), "\t", pugi::format_default, pugi::encoding_utf8)) {
			return { false, "Failed to save XML file." };
		}

		return { true, existing ? "updated" : "appended" };
	}

	static sol::table storageForScript(sol::this_state ts, const std::string& name) {
		sol::state_view lua(ts);
		sol::table storage = lua.create_table();

		namespace fs = std::filesystem;
		
		// Determine potential base directories
		fs::path scriptsPath = fs::weakly_canonical(LuaScriptManager::getInstance().getScriptsDirectory());
		fs::path dataPath = fs::weakly_canonical(FileSystem::GetDataDirectory().ToStdString());

		std::string scriptDir = lua["SCRIPT_DIR"].get_or(std::string(""));

		// Ensure scriptDir is safe and canonical
		fs::path canonicalScriptDir = fs::weakly_canonical(scriptDir);
		auto scriptDirRel = canonicalScriptDir.lexically_relative(scriptsPath);
		if (scriptDirRel.empty() || scriptDirRel.string().find("..") != std::string::npos) {
			scriptDir = scriptsPath.string(); 
		} else {
			scriptDir = canonicalScriptDir.string();
		}

		fs::path fullPath;
		bool allowed = false;

		try {
			fs::path p(name);
			if (p.is_absolute()) {
				fullPath = fs::weakly_canonical(p);
				// Check if absolute path is within allowed roots
				std::vector<fs::path> allowedRoots = { scriptsPath, dataPath };
				for (const auto& root : allowedRoots) {
					auto relative = fullPath.lexically_relative(root);
					if (!relative.empty() && relative.string().find("..") == std::string::npos) {
						allowed = true;
						break;
					}
				}
			} else {
				// For relative paths, try anchoring to each root (scriptDir first)
				std::vector<fs::path> roots = { fs::path(scriptDir), scriptsPath, dataPath };
				for (const auto& root : roots) {
					fs::path candidate = fs::weakly_canonical(root / p);
					auto relative = candidate.lexically_relative(root);
					if (!relative.empty() && relative.string().find("..") == std::string::npos) {
						fullPath = candidate;
						allowed = true;
						break;
					}
				}
			}
		} catch (...) {
			printf("[Lua Security] Failed to canonicalize path in app.storage: %s\n", name.c_str());
			return lua.create_table();
		}

		if (!allowed) {
			printf("[Lua Security] Blocked unsafe path in app.storage: %s\n", name.c_str());
			return lua.create_table();
		}

		std::string path = fullPath.string();
		storage["path"] = path;

		storage["load"] = [path](sol::this_state ts2, sol::object) -> sol::object {
			sol::state_view lua(ts2);
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file.is_open()) {
				return sol::make_object(lua, sol::nil);
			}

			const std::streamsize fileSize = file.tellg();
			if (fileSize <= 0 || fileSize > static_cast<std::streamsize>(4 * 1024 * 1024)) {
				return sol::make_object(lua, sol::nil);
			}

			std::string content(static_cast<size_t>(fileSize), '\0');
			file.seekg(0, std::ios::beg);
			if (!file.read(content.data(), fileSize)) {
				return sol::make_object(lua, sol::nil);
			}

			sol::table json = lua["json"];
			if (!json.valid() || !json["decode"].valid()) {
				return sol::make_object(lua, sol::nil);
			}

			try {
				sol::function decode = json["decode"];
				sol::protected_function_result result = decode(content);
				if (!result.valid()) {
					return sol::make_object(lua, sol::nil);
				}
				sol::object decoded = result;
				return sol::make_object(lua, decoded);
			} catch (const sol::error&) {
				return sol::make_object(lua, sol::nil);
			}
		};

		storage["exists"] = [path](sol::object) -> bool {
			return std::filesystem::exists(path);
		};

		storage["readText"] = [path](sol::this_state ts2, sol::object) -> std::tuple<sol::object, sol::object> {
			sol::state_view lua(ts2);

			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file.is_open()) {
				return {
					sol::make_object(lua, sol::nil),
					sol::make_object(lua, std::string("Could not open file."))
				};
			}

			const std::streamsize fileSize = file.tellg();
			if (fileSize < 0 || fileSize > static_cast<std::streamsize>(4 * 1024 * 1024)) {
				return {
					sol::make_object(lua, sol::nil),
					sol::make_object(lua, std::string("File is too large to read safely."))
				};
			}

			std::string content(static_cast<size_t>(fileSize), '\0');
			file.seekg(0, std::ios::beg);
			if (fileSize > 0 && !file.read(content.data(), fileSize)) {
				return {
					sol::make_object(lua, sol::nil),
					sol::make_object(lua, std::string("Could not read file contents."))
				};
			}

			return {
				sol::make_object(lua, content),
				sol::make_object(lua, sol::nil)
			};
		};

		storage["save"] = [path](sol::this_state ts2, sol::object first, sol::object second) -> bool {
			sol::state_view lua(ts2);
			std::string content;

			sol::object data = (second.valid() && !second.is<sol::nil_t>()) ? second : first;
			if (!data.valid() || data.is<sol::nil_t>()) {
				return false;
			}

			if (data.is<std::string>()) {
				content = data.as<std::string>();
			} else {
				sol::table json = lua["json"];
				if (!json.valid() || !json["encode_pretty"].valid()) {
					return false;
				}
				try {
					sol::function encode = json["encode_pretty"];
					sol::protected_function_result result = encode(data);
					if (!result.valid()) {
						return false;
					}
					content = result.get<std::string>();
				} catch (const sol::error&) {
					return false;
				}
			}

			std::ofstream file(path, std::ios::trunc);
			if (!file.is_open()) {
				return false;
			}
			file << content;
			file.close();
			return true;
		};

		storage["clear"] = [path](sol::object) -> bool {
			return std::remove(path.c_str()) == 0;
		};

		storage["upsertXml"] = [path](sol::object, sol::table options) -> std::tuple<bool, std::string> {
			return upsertXmlNodeFile(path, options);
		};

		return storage;
	}

	// ============================================================================
	// Register App API
	// ============================================================================

	void registerApp(sol::state& lua) {
		// Create the 'app' table
		sol::table app = lua.create_named_table("app");

		// Version info
		app["version"] = __RME_VERSION__;
		app["apiVersion"] = 1;

		// Functions
		app["alert"] = showAlert;
		app["chooseFile"] = chooseFile;
		app["hasMap"] = hasMap;
		app["refresh"] = refresh;
		app["setBrush"] = [](const std::string& name) {
			Brush* b = g_brushes.getBrush(name);
			if (b) {
				g_gui.SelectBrush(b);
			}
		};
		app["transaction"] = transaction;
		app["setClipboard"] = setClipboard;
		app["getDataDirectory"] = getDataDirectory;
		app["addContextMenu"] = [](sol::this_state ts, const std::string& label, sol::function callback) {
			g_luaScripts.registerContextMenuItem(label, callback, ts);
		};
		app["selectRaw"] = [](int itemId) {
			if (g_items.typeExists(itemId)) {
				ItemType it = g_items[itemId];
				if (it.raw_brush) {
					g_gui.SelectBrush(it.raw_brush, TILESET_RAW);
				}
			}
		};

		app["setCameraPosition"] = [](int x, int y, int z) {
			g_gui.SetScreenCenterPosition(Position(x, y, z));
		};
		app["storage"] = storageForScript;

		// Yield to process pending UI events (prevents UI freeze during long operations)
		app["yield"] = []() {
			if (wxTheApp && !LuaTransaction::getInstance().isActive()) {
				wxTheApp->Yield(true);
			}
		};

		// Sleep for a given number of milliseconds (use sparingly, blocks the UI)
		app["sleep"] = [](int milliseconds) {
			if (milliseconds > 0 && milliseconds <= 10000) { // Max 10 seconds
				wxMilliSleep(static_cast<unsigned long>(milliseconds));
			}
		};

		// Get elapsed time in milliseconds since application start (high precision timer)
		app["getTime"] = []() -> long {
			return g_gui.gfx.getElapsedTime();
		};

		// Event system: app.events:on("eventName", callback) / app.events:off(id)
		sol::table events = lua.create_table();
		events["on"] = [](sol::this_state ts, sol::table self, const std::string& eventName, sol::function callback) -> int {
			return g_luaScripts.addEventListener(eventName, callback, ts);
		};
		events["off"] = [](sol::this_state ts, sol::table self, int listenerId) -> bool {
			return g_luaScripts.removeEventListener(listenerId);
		};
		app["events"] = events;

		// Properties via metatable (for dynamic properties like 'map' and 'selection')
		sol::table mt = lua.create_table();
		mt[sol::meta_function::index] = [](sol::this_state ts, sol::table self, std::string key) -> sol::object {
			sol::state_view lua(ts);

			if (key == "map") {
				Map* map = getMap();
				if (map) {
					return sol::make_object(lua, map);
				}
				return sol::nil;
			} else if (key == "selection") {
				Selection* sel = getSelection();
				if (sel) {
					return sol::make_object(lua, sel);
				}
				return sol::nil;
			} else if (key == "borders") {
				return getBorders(ts);
			} else if (key == "grounds") {
				return getGrounds(ts);
			} else if (key == "walls") {
				return getWalls(ts);
			} else if (key == "editor") {
				Editor* editor = g_gui.GetCurrentEditor();
				if (editor) {
					return sol::make_object(lua, editor);
				}
				return sol::nil;
			} else if (key == "brush") {
				Brush* b = g_gui.GetCurrentBrush();
				if (b) {
					return sol::make_object(lua, b);
				}
				return sol::nil;
			} else if (key == "brushSize") {
				return sol::make_object(lua, g_gui.GetBrushSize());
			} else if (key == "brushShape") {
				return sol::make_object(lua, g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE ? "circle" : "square");
			} else if (key == "brushVariation") {
				return sol::make_object(lua, g_gui.GetBrushVariation());
			} else if (key == "spawnTime") {
				return sol::make_object(lua, g_gui.GetSpawnTime());
			}
			return sol::nil;
		};

		mt[sol::meta_function::new_index] = [](sol::this_state ts, sol::table self, std::string key, sol::object value) {
			if (key == "brushSize") {
				if (value.is<int>()) {
					g_gui.SetBrushSize(value.as<int>());
				}
			} else if (key == "brushShape") {
				if (value.is<std::string>()) {
					std::string s = value.as<std::string>();
					if (s == "circle") {
						g_gui.SetBrushShape(BRUSHSHAPE_CIRCLE);
					} else if (s == "square") {
						g_gui.SetBrushShape(BRUSHSHAPE_SQUARE);
					}
				}
			} else if (key == "brushVariation") {
				if (value.is<int>()) {
					g_gui.SetBrushVariation(value.as<int>());
				}
			} else if (key == "spawnTime") {
				if (value.is<int>()) {
					g_gui.SetSpawnTime(value.as<int>());
				}
			}
		};

		// Keyboard modification state
		sol::table keyboard = lua.create_table();
		keyboard["isCtrlDown"] = []() -> bool {
			return wxGetKeyState(WXK_CONTROL);
		};
		keyboard["isShiftDown"] = []() -> bool {
			return wxGetKeyState(WXK_SHIFT);
		};
		keyboard["isAltDown"] = []() -> bool {
			return wxGetKeyState(WXK_ALT);
		};
		app["keyboard"] = keyboard;

		// Clipboard / Edit operations
		app["copy"] = []() { g_gui.DoCopy(); };
		app["cut"] = []() { g_gui.DoCut(); };
		app["paste"] = []() { g_gui.DoPaste(); };

		// Map overlay system
		sol::table mapView = lua.create_table();
		mapView["addOverlay"] = [](sol::this_state ts, sol::variadic_args va) -> bool {
			if (va.size() == 2 && va[0].is<std::string>() && va[1].is<sol::table>()) {
				return g_luaScripts.addMapOverlay(va[0].as<std::string>(), va[1].as<sol::table>(), ts);
			}
			if (va.size() == 3 && va[1].is<std::string>() && va[2].is<sol::table>()) {
				return g_luaScripts.addMapOverlay(va[1].as<std::string>(), va[2].as<sol::table>(), ts);
			}
			return false;
		};
		mapView["removeOverlay"] = [](sol::this_state ts, sol::variadic_args va) -> bool {
			if (va.size() == 1 && va[0].is<std::string>()) {
				return g_luaScripts.removeMapOverlay(va[0].as<std::string>());
			}
			if (va.size() == 2 && va[1].is<std::string>()) {
				return g_luaScripts.removeMapOverlay(va[1].as<std::string>());
			}
			return false;
		};
		mapView["setEnabled"] = [](sol::this_state ts, sol::variadic_args va) -> bool {
			if (va.size() == 2 && va[0].is<std::string>() && va[1].is<bool>()) {
				return g_luaScripts.setMapOverlayEnabled(va[0].as<std::string>(), va[1].as<bool>());
			}
			if (va.size() == 3 && va[1].is<std::string>() && va[2].is<bool>()) {
				return g_luaScripts.setMapOverlayEnabled(va[1].as<std::string>(), va[2].as<bool>());
			}
			return false;
		};
		mapView["registerShow"] = [](sol::this_state ts, sol::variadic_args va) -> bool {
			std::string label;
			std::string overlayId;
			bool enabled = true;
			sol::function ontoggle;

			if (va.size() == 2 && va[0].is<std::string>() && va[1].is<std::string>()) {
				label = va[0].as<std::string>();
				overlayId = va[1].as<std::string>();
			} else if (va.size() >= 3) {
				if (va[0].is<sol::table>()) {
					if (va[1].is<std::string>()) {
						label = va[1].as<std::string>();
					}
					if (va[2].is<std::string>()) {
						overlayId = va[2].as<std::string>();
					}
				} else if (va[0].is<std::string>() && va[1].is<std::string>()) {
					label = va[0].as<std::string>();
					overlayId = va[1].as<std::string>();
				}
			}

			if (va.size() >= 3 && va[va.size() - 1].is<sol::table>()) {
				sol::table opts = va[va.size() - 1].as<sol::table>();
				enabled = opts.get_or(std::string("enabled"), enabled);
				if (opts["ontoggle"].valid()) {
					ontoggle = opts["ontoggle"];
				}
			} else if (va.size() >= 3 && va[va.size() - 1].is<bool>()) {
				enabled = va[va.size() - 1].as<bool>();
			}

			if (label.empty() || overlayId.empty()) {
				return false;
			}

			return g_luaScripts.registerMapOverlayShow(label, overlayId, enabled, ontoggle, ts);
		};
		app["mapView"] = mapView;

		app[sol::metatable_key] = mt;

		// Register Editor usertype for undo/redo functionality
		lua.new_usertype<Editor>(
			"Editor",
			sol::no_constructor,

			// Undo/Redo functions
			"undo", [](Editor* editor) {
			if (editor && editor->actionQueue && editor->actionQueue->canUndo()) {
				editor->actionQueue->undo();
				g_gui.RefreshView();
			} },
			"redo", [](Editor* editor) {
			if (editor && editor->actionQueue && editor->actionQueue->canRedo()) {
				editor->actionQueue->redo();
				g_gui.RefreshView();
			} },
			"canUndo", [](Editor* editor) -> bool { return editor && editor->actionQueue && editor->actionQueue->canUndo(); },
			"canRedo", [](Editor* editor) -> bool { return editor && editor->actionQueue && editor->actionQueue->canRedo(); },

			// History info
			"historyIndex", sol::property([](Editor* editor) -> int {
				if (editor && editor->actionQueue) {
					return (int)editor->actionQueue->getCurrentIndex();
				}
				return 0;
			}),
			"historySize", sol::property([](Editor* editor) -> int {
				if (editor && editor->actionQueue) {
					return (int)editor->actionQueue->getSize();
				}
				return 0;
			}),

			// Get history as a table
			"getHistory", [](Editor* editor, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table history = lua.create_table();

			if (editor && editor->actionQueue) {
				size_t size = editor->actionQueue->getSize();
				for (size_t i = 0; i < size; ++i) {
					sol::table input = lua.create_table();
					input["index"] = (int)(i + 1); // 1-based for Lua
					input["name"] = editor->actionQueue->getActionName(i);
					history[i + 1] = input;
				}
			}
			return history; },

			// Navigate to specific history index
			"goToHistory", [](Editor* editor, int targetIndex) {
			if (!editor || !editor->actionQueue) return;

			int current = (int)editor->actionQueue->getCurrentIndex();
			int target = targetIndex; // Already 1-based from Lua

			if (target < 0) target = 0;
			if (target > (int)editor->actionQueue->getSize()) {
				target = (int)editor->actionQueue->getSize();
			}

			int diff = target - current;

			if (diff > 0) {
				for (int i = 0; i < diff; ++i) {
					if (editor->actionQueue->canRedo()) {
						editor->actionQueue->redo();
					}
				}
			} else if (diff < 0) {
				for (int i = 0; i < -diff; ++i) {
					if (editor->actionQueue->canUndo()) {
						editor->actionQueue->undo();
					}
				}
			}
			g_gui.RefreshView(); }
		);
	}

} // namespace LuaAPI
