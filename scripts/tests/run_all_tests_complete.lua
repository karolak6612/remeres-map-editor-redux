-- @Title: RME Redux - Complete Lua API Test Suite
-- @Description: Comprehensive test suite with detailed output and correct counting.
-- @Usage: Run from Scripts menu or via hotkey
-- ============================================================================

-- Test configuration
local TEST_CONFIG = {
    show_individual_tests = false,  -- Set to true to see each test result
}

-- Test results storage
local results = {
    files = {},
    failed_tests = {},
}

-- Test File List
local test_files = {
    -- Core API Tests
    { name = "test_app.lua", category = "Core API" },
    { name = "test_map.lua", category = "Core API" },
    { name = "test_tile_item.lua", category = "Core API" },
    { name = "test_position.lua", category = "Core API" },
    { name = "test_selection.lua", category = "Core API" },
    { name = "test_creature.lua", category = "Core API" },
    { name = "test_brush.lua", category = "Core API" },
    { name = "test_image.lua", category = "Core API" },
    { name = "test_ui.lua", category = "Core API" },
    { name = "test_http.lua", category = "Core API" },
    
    -- Extended API Tests
    { name = "test_color.lua", category = "Extended API" },
    { name = "test_json.lua", category = "Extended API" },
    { name = "test_http_extended.lua", category = "Extended API" },
    { name = "test_noise_extended.lua", category = "Extended API" },
    { name = "test_algo_extended.lua", category = "Extended API" },
    { name = "test_geo_extended.lua", category = "Extended API" },
    { name = "test_dialog_extended.lua", category = "Extended API" },
    { name = "test_items.lua", category = "Extended API" },
    
    -- Compatibility Tests
    { name = "test_noise.lua", category = "Compatibility" },
    { name = "test_algo.lua", category = "Compatibility" },
    { name = "test_geo.lua", category = "Compatibility" },
}

-- Statistics
local stats = {
    total_files = 0,
    passed_files = 0,
    failed_files = 0,
}

-- ============================================================================
-- Test Runner
-- ============================================================================

local function runTestFile(test_file)
    -- Ensure framework exists
    if not _G.framework then
        _G.framework = require("framework")
    end
    
    -- Get counts before running
    local prev_passed = _G.framework.tests_passed or 0
    local prev_failed = _G.framework.tests_failed or 0
    
    -- Run the test file
    local status, err = pcall(function()
        dofile(test_file.name)
    end)
    
    -- Get counts after running
    local curr_passed = _G.framework.tests_passed or 0
    local curr_failed = _G.framework.tests_failed or 0
    
    -- Calculate file stats
    local file_passed = curr_passed - prev_passed
    local file_failed = curr_failed - prev_failed
    local file_total = file_passed + file_failed
    
    -- Store results
    test_file.passed = file_passed
    test_file.failed = file_failed
    test_file.status = (status and file_failed == 0) and "passed" or "failed"
    
    if not status then
        print("  [ERROR] " .. test_file.name .. ": " .. tostring(err))
    elseif file_failed > 0 then
        print("  [FAIL]  " .. test_file.name .. " (" .. file_passed .. " passed, " .. file_failed .. " failed)")
    else
        print("  [PASS]  " .. test_file.name .. " (" .. file_passed .. " tests)")
    end
    
    stats.total_files = stats.total_files + 1
    if file_failed == 0 and status then
        stats.passed_files = stats.passed_files + 1
    else
        stats.failed_files = stats.failed_files + 1
    end
    
    return {
        name = test_file.name,
        passed = file_passed,
        failed = file_failed,
        status = test_file.status,
    }
end

local function runAllTests()
    print("")
    print("===============================================================")
    print("         RME REDUX - COMPLETE LUA API TEST SUITE")
    print("===============================================================")
    print("")
    print("Total test files: " .. #test_files)
    print("")
    
    -- Group tests by category
    local categories = {}
    for _, test_file in ipairs(test_files) do
        if not categories[test_file.category] then
            categories[test_file.category] = {}
        end
        table.insert(categories[test_file.category], test_file)
    end
    
    -- Run tests by category
    for category, files in pairs(categories) do
        print("")
        print("--- " .. category .. " ---")
        print("")
        
        for _, test_file in ipairs(files) do
            print("Running: " .. test_file.name)
            runTestFile(test_file)
        end
    end
    
    -- Generate report
    print("")
    print("===============================================================")
    print("                      TEST SUMMARY")
    print("===============================================================")
    print("")
    print("Files: " .. stats.passed_files .. "/" .. stats.total_files .. " passed")
    print("")
    print("Total Tests: " .. (_G.framework.tests_passed or 0) + (_G.framework.tests_failed or 0))
    print("  Passed: " .. (_G.framework.tests_passed or 0))
    print("  Failed: " .. (_G.framework.tests_failed or 0))
    print("")
    
    local total = (_G.framework.tests_passed or 0) + (_G.framework.tests_failed or 0)
    local pass_rate = total > 0 and ((_G.framework.tests_passed or 0) / total * 100) or 0
    print("Pass Rate: " .. string.format("%.1f%%", pass_rate))
    print("")
    
    print("===============================================================")
    
    if _G.framework.tests_failed == 0 then
        print("SUCCESS: ALL TESTS PASSED!")
        print("")
        print("Total: " .. (_G.framework.tests_passed or 0) .. " tests passed")
        return true
    else
        print("WARNING: " .. (_G.framework.tests_failed or 0) .. " test(s) failed!")
        return false
    end
end

-- ============================================================================
-- Main
-- ============================================================================

-- Reset framework counter at the start
local framework = require("framework")
framework.tests_passed = 0
framework.tests_failed = 0

print("")
print("===============================================================")
print("           RME REDUX - LUA API TEST SUITE")
print("===============================================================")
print("")
print("Running comprehensive tests for ALL Lua APIs...")
print("")

-- Check if we have a map open
if app and app.hasMap then
    if app:hasMap() then
        print("[OK] Map is open - Full test suite available")
    else
        print("[WARN] No map open - Some tests will be skipped")
    end
else
    print("[INFO] Cannot check map status")
end

-- Run all tests
local success = runAllTests()

return success
