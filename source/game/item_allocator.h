#ifndef RME_GAME_ITEM_ALLOCATOR_H_
#define RME_GAME_ITEM_ALLOCATOR_H_

#include <cstddef>

class ItemAllocator {
public:
	static void* allocate(size_t size);
	static void deallocate(void* ptr, size_t size);
};

#endif
