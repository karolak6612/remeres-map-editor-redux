#include <gc.h>
#include <new>
#include <cstdlib>

// Static initializer to ensure GC is initialized before main/wxEntry
struct GCInitializer {
    GCInitializer() {
        GC_INIT();
    }
};

static GCInitializer gc_init;

// Global overrides
void* operator new(size_t size) {
    void* ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    GC_FREE(ptr);
}

void operator delete[](void* ptr) noexcept {
    GC_FREE(ptr);
}

// Sized delete (C++14)
void operator delete(void* ptr, size_t size) noexcept {
    GC_FREE(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    GC_FREE(ptr);
}

// Debug new support (overloading the signature used by debug new macros)
void* operator new(size_t size, const char* /*file*/, int /*line*/) {
    void* ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](size_t size, const char* /*file*/, int /*line*/) {
    void* ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

// Matching delete for placement new (called if constructor throws)
void operator delete(void* ptr, const char* /*file*/, int /*line*/) noexcept {
    GC_FREE(ptr);
}

void operator delete[](void* ptr, const char* /*file*/, int /*line*/) noexcept {
    GC_FREE(ptr);
}
