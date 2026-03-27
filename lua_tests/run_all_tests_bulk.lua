-- @Title: RME Redux - Quick Bulk Test Runner
-- @Description: Simple script to run all tests quickly with minimal output
-- @Usage: Scripts → Run Script → run_all_tests_bulk.lua

-- Reset framework counter at the start
local framework = require("framework")
framework.tests_passed = 0
framework.tests_failed = 0

-- Test files to run
local test_files = {
    -- Core
    "test_app.lua",
    "test_map.lua",
    "test_tile_item.lua",
    "test_position.lua",
    "test_selection.lua",
    "test_creature.lua",
    "test_brush.lua",
    "test_image.lua",
    "test_ui.lua",
    "test_http.lua",
    
    -- Extended
    "test_color.lua",
    "test_json.lua",
    "test_http_extended.lua",
    "test_noise_extended.lua",
    "test_algo_extended.lua",
    "test_geo_extended.lua",
    "test_dialog_extended.lua",
    "test_items.lua",
    
    -- Compatibility
    "test_noise.lua",
    "test_algo.lua",
    "test_geo.lua",
}

-- Store failed test details
local all_failed_tests = {}

print("")
print("═══════════════════════════════════════════════════════════")
print("        RME REDUX - BULK TEST RUNNER")
print("═══════════════════════════════════════════════════════════")
print("")

local start_time = os and os.time() or 0
local completed = 0
local failed_files = {}
local total_failures = 0
local passed_files = 0

-- Track if ANY test in the file failed
local file_had_failures = false

for i, file in ipairs(test_files) do
    print(string.format("[%d/%d] %s ... ", i, #test_files, file))
    
    -- Get current failure count before running file
    local prev_failed = framework.tests_failed or 0
    local prev_passed = framework.tests_passed or 0
    
    -- Store failed tests for this file
    local file_failed_tests = {}
    
    -- Wrap framework.test to capture failures
    local original_test = framework.test
    framework.test = function(name, func)
        local status, err = pcall(func)
        if not status then
            table.insert(file_failed_tests, {name = name, error = err})
            framework.tests_failed = framework.tests_failed + 1
        else
            framework.tests_passed = framework.tests_passed + 1
        end
    end
    
    local status, err = pcall(function()
        dofile(file)
    end)
    
    -- Restore original test function
    framework.test = original_test
    
    completed = completed + 1
    
    -- Calculate failures from this file
    local file_failures = (framework.tests_failed or 0) - prev_failed
    total_failures = total_failures + file_failures
    
    -- Store failed test details
    if #file_failed_tests > 0 then
        for _, test in ipairs(file_failed_tests) do
            table.insert(all_failed_tests, {file = file, name = test.name, error = test.error})
        end
    end
    
    -- Check if this specific file had failures
    file_had_failures = (file_failures > 0) or not status
    
    if status and not file_had_failures then
        print("  ✓ PASS")
        passed_files = passed_files + 1
    elseif status then
        print("  ✗ FAIL (" .. file_failures .. " failed)")
        table.insert(failed_files, file)
    else
        print("  ERROR: " .. tostring(err))
        table.insert(failed_files, file .. " (load error)")
    end
end

local end_time = os and os.time() or 0
local duration = end_time - start_time

print("")
print("───────────────────────────────────────────────────────────")
print("SUMMARY")
print("───────────────────────────────────────────────────────────")
print("Files passed:    " .. passed_files .. "/" .. #test_files)
print("Tests passed:    " .. (framework.tests_passed or 0))
print("Tests failed:    " .. total_failures)
if duration > 0 then
    print("Duration:        " .. duration .. " seconds")
end

if #failed_files > 0 then
    print("")
    print("Failed files:")
    for _, file in ipairs(failed_files) do
        print("  • " .. file)
    end
    
    print("")
    print("Failed tests details:")
    for _, test in ipairs(all_failed_tests) do
        print("  • " .. test.file .. " - " .. test.name)
        print("    Error: " .. tostring(test.error))
    end
end

print("")
print("═══════════════════════════════════════════════════════════")

if total_failures == 0 then
    print("✓ ALL TESTS PASSED!")
else
    print("⚠ " .. total_failures .. " test(s) failed")
end
print("═══════════════════════════════════════════════════════════")
print("")

return total_failures == 0
