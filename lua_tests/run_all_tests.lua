-- @Title: RME API Test Suite
-- @Description: Runs all API verification tests.
-- This test suite covers ALL Lua API calls in RME Redux.

local framework = require("framework")

framework.clear_log()

print("Starting RME Lua API Tests...")
print("This suite tests EVERY Lua API call available in RME Redux.")
print("")

-- Core API tests (basic functionality)
local core_tests = {
    "test_app.lua",           -- app namespace, transactions, events, overlays
    "test_map.lua",           -- Map object, tiles iterator, spawns iterator
    "test_tile_item.lua",     -- Tile and Item operations
    "test_position.lua",      -- Position constructors, operators, methods
    "test_selection.lua",     -- Selection operations
    "test_creature.lua",      -- Creature, Spawn, Direction enum
    "test_brush.lua",         -- Brush properties and methods
    "test_image.lua",         -- Image loading, resizing
    "test_ui.lua",            -- Dialog and widgets (basic)
    "test_http.lua"           -- HTTP basic methods
}

-- Extended API tests (comprehensive coverage)
local extended_tests = {
    "test_color.lua",         -- Color constructors, lighten/darken, predefined colors
    "test_json.lua",          -- JSON encode/decode, roundtrip
    "test_http_extended.lua", -- HTTP streaming, security, edge cases
    "test_noise_extended.lua",-- All noise functions, utilities, batch generation
    "test_algo_extended.lua", -- Cellular automata, erosion, maze, dungeon generation
    "test_geo_extended.lua",  -- Bresenham, bezier, flood fill, shapes, distances
    "test_dialog_extended.lua",-- All dialog widgets and layout methods
    "test_items.lua"          -- Items namespace, search, info lookup
}

-- Original basic tests (kept for compatibility)
local basic_tests = {
    "test_noise.lua",         -- Basic noise tests
    "test_algo.lua",          -- Basic algorithm tests
    "test_geo.lua"            -- Basic geometry tests
}

print("=== CORE API TESTS ===")
for _, script in ipairs(core_tests) do
    print("\n>>> Running " .. script .. " <<<")
    local status, err = pcall(function()
        dofile(script)
    end)
    if not status then
        print("CRITICAL ERROR in " .. script .. ": " .. tostring(err))
    end
end

print("\n=== EXTENDED API TESTS ===")
for _, script in ipairs(extended_tests) do
    print("\n>>> Running " .. script .. " <<<")
    local status, err = pcall(function()
        dofile(script)
    end)
    if not status then
        print("CRITICAL ERROR in " .. script .. ": " .. tostring(err))
    end
end

print("\n=== BASIC COMPATIBILITY TESTS ===")
for _, script in ipairs(basic_tests) do
    print("\n>>> Running " .. script .. " <<<")
    local status, err = pcall(function()
        dofile(script)
    end)
    if not status then
        print("CRITICAL ERROR in " .. script .. ": " .. tostring(err))
    end
end

print("\n========================================")
print("ALL TESTS COMPLETED")
print("")
print("Summary:")
print("  Passed: " .. framework.tests_passed)
print("  Failed: " .. framework.tests_failed)
print("")
print("Check scripts/tests/test_results.log for details.")
print("========================================")

-- Exit with error code if tests failed
if framework.tests_failed > 0 then
    error("Some tests failed!")
end
