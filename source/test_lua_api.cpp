//////////////////////////////////////////////////////////////////////
// Unit tests for Lua API
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <cmath>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "lua/lua_api.h"
#include "map/map.h"
#include "map/tile.h"
#include "game/item.h"
#include "game/items.h"
#include "game/creature.h"
#include "game/spawn.h"
#include "map/position.h"
#include "app/application.h"

// Setup minimal item database for testing
void setup_test_items() {
	g_items.clear();
	g_items.items.resize(2000);

	// Item 100: Simple item
	g_items.items[100] = std::make_unique<ItemType>();
	g_items.items[100]->id = 100;
	g_items.items[100]->name = "test item";

	// Item 101: Stackable
	g_items.items[101] = std::make_unique<ItemType>();
	g_items.items[101]->id = 101;
	g_items.items[101]->stackable = true;
	g_items.items[101]->name = "gold coin";

    // Item 102: Container
    g_items.items[102] = std::make_unique<ItemType>();
    g_items.items[102]->id = 102;
    g_items.items[102]->type = ITEM_TYPE_CONTAINER;
    g_items.items[102]->group = ITEM_GROUP_CONTAINER;
    g_items.items[102]->name = "backpack";
}

void test_position(sol::state& lua) {
	std::cout << "Testing Position API..." << std::endl;

	lua.script(R"LUA(
		local pos = Position(100, 200, 7)
		assert(pos.x == 100)
		assert(pos.y == 200)
		assert(pos.z == 7)

		pos.x = 101
		assert(pos.x == 101)

		local pos2 = Position(101, 200, 7)
		assert(pos == pos2)

		local pos3 = pos + Position(1, 1, 0)
		assert(pos3.x == 102)
		assert(pos3.y == 201)
		assert(pos3.z == 7)

		assert(tostring(pos) == "Position(101, 200, 7)")
	)LUA");

	std::cout << "Position API PASSED" << std::endl;
}

void test_color(sol::state& lua) {
	std::cout << "Testing Color API..." << std::endl;

	lua.script(R"LUA(
		local c = Color(255, 0, 0)
		assert(c.r == 255)
		assert(c.g == 0)
		assert(c.b == 0)
		assert(c.a == 255)

		local c2 = Color(0, 255, 0, 128)
		assert(c2.a == 128)

		-- Test static methods (backward compatibility)
		local c3 = Color.rgb(0, 0, 255)
		assert(c3.b == 255)

		local c4 = Color.hex("#FF0000")
		assert(c4.r == 255)

		local c5 = Color.lighten(Color.black, 50)
		assert(c5.r > 0)

		local c6 = Color.darken(Color.white, 50)
		assert(c6.r < 255)

		assert(Color.red.r == 220)
	)LUA");

	std::cout << "Color API PASSED" << std::endl;
}

void test_item(sol::state& lua) {
	std::cout << "Testing Item API..." << std::endl;

	// Create an item and pass it to Lua
	std::unique_ptr<Item> item = Item::Create(100);
	lua["testItem"] = item.get();

	lua.script(R"LUA(
		assert(testItem:getID() == 100)
		assert(testItem:getName() == "test item")
		assert(not testItem:isStackable())
		assert(not testItem:isContainer())

		testItem:setActionID(1000)
		assert(testItem:getActionID() == 1000)

		testItem:setUniqueID(2000)
		assert(testItem:getUniqueID() == 2000)

		-- Attribute tests
		testItem:setAttribute("myKey", "myValue")
		assert(testItem:getAttribute("myKey") == "myValue")

		testItem:setAttribute("myInt", 123)
		assert(testItem:getAttribute("myInt") == 123)
	)LUA");

	std::unique_ptr<Item> stackable = Item::Create(101);
	stackable->setSubtype(50);
	lua["stackableItem"] = stackable.get();

	lua.script(R"LUA(
		assert(stackableItem:isStackable())
		assert(stackableItem:getCount() == 50)
		stackableItem:setCount(75)
		assert(stackableItem:getCount() == 75)
	)LUA");

	std::cout << "Item API PASSED" << std::endl;
}

void test_json(sol::state& lua) {
	std::cout << "Testing JSON API..." << std::endl;

	lua.script(R"LUA(
		local data = { name = "test", value = 123, list = {1, 2, 3} }
		local str = json.encode(data)
		assert(type(str) == "string")

		local decoded = json.decode(str)
		assert(decoded.name == "test")
		assert(decoded.value == 123)
		assert(decoded.list[1] == 1)
		assert(decoded.list[3] == 3)
	)LUA");

	std::cout << "JSON API PASSED" << std::endl;
}

void test_map_tile(sol::state& lua) {
	std::cout << "Testing Map & Tile API..." << std::endl;

	Map map;
	map.initializeEmpty();
	map.setWidth(1000);
	map.setHeight(1000);

	Tile* tile = map.createTile(100, 100, 7);
	std::unique_ptr<Item> item = Item::Create(100);
	tile->addItem(std::move(item));

	lua["testMap"] = &map;

	lua.script(R"LUA(
		assert(testMap:getWidth() == 1000)
		assert(testMap:getHeight() == 1000)

		local tile = testMap:getTile(100, 100, 7)
		assert(tile ~= nil)
		assert(tile:getPosition().x == 100)

		local items = tile:getItems()
		assert(#items == 1)
		assert(items[1]:getID() == 100)

		local tile2 = testMap:getTile(200, 200, 7)
		assert(tile2 == nil)
	)LUA");

	std::cout << "Map & Tile API PASSED" << std::endl;
}

void test_creature(sol::state& lua) {
	std::cout << "Testing Creature API..." << std::endl;

	// Initialize a creature type
	g_creatures.addCreatureType("Demon", false, Outfit());

	// Create a dummy creature
	Creature creature("Demon");
	creature.setSpawnTime(120);
	creature.setDirection(EAST);

	lua["testCreature"] = &creature;

	lua.script(R"LUA(
		assert(testCreature.name == "Demon")
		assert(testCreature.spawnTime == 120)

		testCreature.spawnTime = 240
		assert(testCreature.spawnTime == 240)

		assert(testCreature.direction == Direction.EAST)
		testCreature.direction = Direction.SOUTH
		assert(testCreature.direction == Direction.SOUTH)
	)LUA");

	std::cout << "Creature API PASSED" << std::endl;
}

void test_geo(sol::state& lua) {
	std::cout << "Testing Geo API..." << std::endl;

	// Just test existence and basic logic if stateless
	lua.script(R"LUA(
		local dist = geo.distance(Position(0,0,0), Position(10,0,0))
		assert(dist == 10)
	)LUA");

	std::cout << "Geo API PASSED" << std::endl;
}

void test_algo(sol::state& lua) {
	std::cout << "Testing Algo API..." << std::endl;

	lua.script(R"LUA(
		local r = algo.random(1, 10)
		assert(r >= 1 and r <= 10)
	)LUA");

	std::cout << "Algo API PASSED" << std::endl;
}

void test_noise(sol::state& lua) {
	std::cout << "Testing Noise API..." << std::endl;

	lua.script(R"LUA(
		-- Perlin Noise
		local val1 = noise.perlin(10.0, 20.0, 12345, 0.01)
		assert(type(val1) == "number")
		assert(val1 >= -1.0 and val1 <= 1.0)

		-- Simplex Noise
		local val2 = noise.simplex(10.0, 20.0, 12345, 0.01)
		assert(type(val2) == "number")
		assert(val2 >= -1.0 and val2 <= 1.0)

		-- 3D Noise
		local val3 = noise.perlin3d(10.0, 20.0, 30.0, 12345, 0.01)
		assert(type(val3) == "number")

		-- FBM
		local val4 = noise.fbm(10.0, 20.0, 12345, {
			frequency = 0.02,
			octaves = 3,
			gain = 0.6
		})
		assert(type(val4) == "number")

		-- Utility
		local norm = noise.normalize(0.0, 0, 100)
		assert(norm >= 0 and norm <= 100)
		assert(noise.clamp(150, 0, 100) == 100)
		assert(noise.lerp(0, 100, 0.5) == 50)
	)LUA");

	std::cout << "Noise API PASSED" << std::endl;
}

// Implement Application methods to satisfy linker (dummy implementation)
Application::~Application() {}
bool Application::OnInit() { return true; }
void Application::OnEventLoopEnter(wxEventLoopBase* loop) {}
void Application::MacOpenFiles(const wxArrayString& fileNames) {}
int Application::OnExit() { return 0; }
int Application::OnRun() { return 0; }
void Application::Unload() {}
bool Application::OnExceptionInMainLoop() { return false; }
void Application::OnUnhandledException() {}
void Application::OnFatalException() {}
void Application::FixVersionDiscrapencies() {}
bool Application::ParseCommandLineMap(wxString& fileName) { return false; }

Application& wxGetApp() {
	static Application app;
	return app;
}

int main() {
	std::cout << "========================================" << std::endl;
	std::cout << "Lua API Unit Tests" << std::endl;
	std::cout << "========================================" << std::endl;

	setup_test_items();

	// Scope lua state to ensure destruction before return
	try {
		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::debug);

		LuaAPI::registerAll(lua);

		test_position(lua);
		test_color(lua);
		test_json(lua);
		test_item(lua);
		test_map_tile(lua);
		test_creature(lua);
		test_geo(lua);
		test_algo(lua);
		test_noise(lua);

		std::cout << "\n========================================" << std::endl;
		std::cout << "ALL LUA TESTS PASSED!" << std::endl;
		std::cout << "========================================" << std::endl;
	} catch (const sol::error& e) {
		std::cerr << "Lua Error: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}
