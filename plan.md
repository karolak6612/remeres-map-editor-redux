# Search phase
- [x] Check Selection::getSelectedTile() size() == 1 UB
- [x] Check SpatialHashGrid::makeKeyFromCell XOR trick UB
- [x] Check Change::Create() raw pointer leak UB
- [x] Check Tile::location raw pointer UB
- [x] Check GameSprite::Image::lastaccess memory ordering Race condition

# Selected items to fix:
1. `Selection::getSelectedTile()` - UB on empty selection in release builds
2. `SpatialHashGrid::makeKeyFromCell` - Potential signed overflow on INT_MIN/MAX
3. `Change::Create()` - Returns raw pointers requiring caller to wrap them in unique_ptr
4. `Image::lastaccess` - `std::atomic<int64_t>` memory ordering in `visit()` and `clean()`
5. `SelectionThread` - Shared state race condition on `bounds_dirty` atomic
