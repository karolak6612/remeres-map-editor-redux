#include "game/item_allocator.h"

#include <vector>
#include <mutex>
#include <map>
#include <memory>
#include <new>
#include <algorithm>

namespace {

class Pool {
public:
	Pool(size_t objectSize, size_t blockSize = 16384) :
		objectSize(std::max(objectSize, sizeof(void*))),
		blockSize(blockSize),
		head(nullptr) {
	}

	Pool(const Pool&) = delete;
	Pool& operator=(const Pool&) = delete;

	~Pool() {
		for (void* block : blocks) {
			::operator delete(block);
		}
	}

	void* allocate() {
		std::lock_guard<std::mutex> lock(mutex);
		if (!head) {
			allocateBlock();
		}
		void* ptr = head;
		head = *static_cast<void**>(head);
		return ptr;
	}

	void deallocate(void* ptr) {
		std::lock_guard<std::mutex> lock(mutex);
		*static_cast<void**>(ptr) = head;
		head = ptr;
	}

private:
	void allocateBlock() {
		size_t count = blockSize / objectSize;
		if (count == 0) {
			count = 1;
		}

		size_t realBlockSize = count * objectSize;
		void* block = ::operator new(realBlockSize);
		blocks.push_back(block);

		char* start = static_cast<char*>(block);
		for (size_t i = 0; i < count - 1; ++i) {
			void* curr = start + i * objectSize;
			void* next = start + (i + 1) * objectSize;
			*reinterpret_cast<void**>(curr) = next;
		}
		*reinterpret_cast<void**>(start + (count - 1) * objectSize) = head;
		head = start;
	}

	size_t objectSize;
	size_t blockSize;
	void* head;
	std::vector<void*> blocks;
	std::mutex mutex;
};

	// Common sizes for RME Items (Item=32, Teleport=40, Podium=48, Container=56)
	// We use static locals for fast access to these common pools without map lookup
	Pool pool32(32);
	Pool pool40(40);
	Pool pool48(48);
	Pool pool56(56);

	// Fallback for other sizes
	std::map<size_t, std::unique_ptr<Pool>> pools;
	std::mutex poolsMutex;

} // namespace

void* ItemAllocator::allocate(size_t size) {
	if (size == 32) {
		return pool32.allocate();
	}
	if (size == 56) {
		return pool56.allocate();
	}
	if (size == 40) {
		return pool40.allocate();
	}
	if (size == 48) {
		return pool48.allocate();
	}

	std::lock_guard<std::mutex> lock(poolsMutex);
	auto it = pools.find(size);
	if (it == pools.end()) {
		it = pools.emplace(size, std::make_unique<Pool>(size)).first;
	}
	return it->second->allocate();
}

void ItemAllocator::deallocate(void* ptr, size_t size) {
	if (size == 32) {
		pool32.deallocate(ptr);
		return;
	}
	if (size == 56) {
		pool56.deallocate(ptr);
		return;
	}
	if (size == 40) {
		pool40.deallocate(ptr);
		return;
	}
	if (size == 48) {
		pool48.deallocate(ptr);
		return;
	}

	std::lock_guard<std::mutex> lock(poolsMutex);
	auto it = pools.find(size);
	if (it != pools.end()) {
		it->second->deallocate(ptr);
	}
}
