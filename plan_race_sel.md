Bugger says: "editor/selection_thread.h — SelectionThread shares state with main thread. Check race conditions on bounds_dirty atomic."
Wait, `SelectionThread` doesn't share `bounds_dirty` with the main thread. It has its *own* `Selection selection;` member.
Wait, `Selection::getSelectedTile()` has `bounds_dirty` as mutable atomic.
Let's see where the main thread bounds_dirty is modified from SelectionThread.
Ah, `SelectionThread` shares state with main thread... wait, no. It modifies `editor.selection` or does it have its own `selection`?
In `SelectionThread::Work()`:
```cpp
void SelectionThread::Work() {
	selection.start(Selection::SUBTHREAD);
...
}
```
`SelectionThread::SelectionThread` initialized `selection(editor)`.
Wait, look at `bounds_dirty` in `Selection::recalculateBounds()`:
```cpp
void Selection::recalculateBounds() const {
	if (!bounds_dirty) {
		return;
	}
    // ...
	cached_min = minPos;
	cached_max = maxPos;
	bounds_dirty = false;
}
```
If multiple threads call `minPosition()`/`maxPosition()`, they could race on `recalculateBounds()`, modifying `cached_min`/`cached_max` concurrently without locking.
Also, the `bounds_dirty = false` is outside a lock. This is a data race on `cached_min`/`cached_max`!
Let's change `bounds_dirty` to be protected by a mutex, or add a mutex to `recalculateBounds()`.
Let's add a `std::mutex bounds_mutex` to `Selection`.
