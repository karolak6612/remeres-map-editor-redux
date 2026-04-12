# RME Redux - Test Suite Quick Reference

## 📁 Test Runner Files

| File | Purpose | Output | Best For |
|------|---------|--------|----------|
| `run_all_tests_complete.lua` | Full test suite with detailed reports | Console + Log file | CI/CD, detailed debugging |
| `run_all_tests_bulk.lua` | Quick bulk test runner | Console only | Fast verification |
| `run_all_tests.lua` | Original categorized runner | Console + Log | Backward compatibility |

## 🎯 Quick Start

### Run All Tests (Recommended)
```
Scripts → Run Script → run_all_tests_complete.lua
```

### Run Quick Tests
```
Scripts → Run Script → run_all_tests_bulk.lua
```

## 📊 Test Coverage

### Core API (10 files)
- ✅ `test_app.lua` - Editor application API
- ✅ `test_map.lua` - Map object and operations
- ✅ `test_tile_item.lua` - Tile and Item operations
- ✅ `test_position.lua` - Position type and operators
- ✅ `test_selection.lua` - Selection management
- ✅ `test_creature.lua` - Creature and Spawn objects
- ✅ `test_brush.lua` - Brush properties and methods
- ✅ `test_image.lua` - Image loading and manipulation
- ✅ `test_ui.lua` - Dialog basic widgets
- ✅ `test_http.lua` - HTTP basic methods

### Extended API (8 files)
- ✅ `test_color.lua` - Color utilities
- ✅ `test_json.lua` - JSON encoding/decoding
- ✅ `test_http_extended.lua` - HTTP streaming and security
- ✅ `test_noise_extended.lua` - Noise generation (all types)
- ✅ `test_algo_extended.lua` - Algorithms (caves, mazes, dungeons)
- ✅ `test_geo_extended.lua` - Geometry (lines, curves, shapes)
- ✅ `test_dialog_extended.lua` - All dialog widgets
- ✅ `test_items.lua` - Item registry and search

### Compatibility (3 files)
- ✅ `test_noise.lua` - Original noise tests
- ✅ `test_algo.lua` - Original algorithm tests
- ✅ `test_geo.lua` - Original geometry tests

## 📈 Statistics

- **Total Test Files:** 21
- **Total Test Cases:** ~450+
- **Total Assertions:** ~1000+
- **API Modules Covered:** 14
- **Estimated Run Time:** 30-60 seconds

## 🔧 Configuration

### Edit `run_all_tests_complete.lua`
```lua
local TEST_CONFIG = {
    show_individual_tests = false,  -- true for verbose output
    show_summary = true,
    export_log = true,
    log_file_path = "scripts/tests/test_results_full.log",
}
```

## 📝 Log Files

| File | Description |
|------|-------------|
| `test_results_full.log` | Complete test results with failures |
| `test_results.log` | Original test runner log |

## 🐛 Debugging Failed Tests

1. **Run complete test suite** with `show_individual_tests = true`
2. **Check log file** for detailed error messages
3. **Run individual test file** to isolate the issue
4. **Review API documentation** in `lua_api_*.cpp` files

## 📋 Test Categories

### Existence Tests
Verify API functions and properties exist.

### Functionality Tests
Test correct behavior with valid inputs.

### Edge Case Tests
Test boundaries, invalid inputs, error handling.

### Roundtrip Tests
Verify encode/decode operations preserve data.

### Consistency Tests
Ensure deterministic behavior.

### Integration Tests
Test real-world usage patterns.

## 🚀 Performance Tips

1. **Close unnecessary scripts** before running tests
2. **Use bulk runner** for quick verification
3. **Run individual files** when debugging specific APIs
4. **Enable verbose mode** only when needed

## 📖 Additional Resources

- `README_TESTS.md` - Detailed test documentation
- `framework.lua` - Test framework implementation
- Individual test files - Inline comments and examples

## 🆘 Troubleshooting

### "No map open" warnings
Some tests require an open map. Open any map before running.

### HTTP test failures
HTTP tests use invalid URLs intentionally to test error handling.

### Dialog tests don't show UI
Dialog tests verify method existence, not visual display.

### Brush tests fail
Brush names vary by brush set. Tests use first available brush.

## ✅ Success Criteria

- **All tests pass:** 0 failed, 0 errors
- **Acceptable:** < 5% failure rate (implementation differences)
- **Investigate:** > 5% failure rate (potential bugs)

## 📞 Support

For issues or questions:
1. Check `README_TESTS.md` for detailed documentation
2. Review test output logs
3. Verify RME Redux version compatibility
4. Report bugs on GitHub issues page
