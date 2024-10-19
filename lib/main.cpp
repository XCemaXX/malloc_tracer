#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <cstdint>
#include <exception>
#include <new>

#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>

//#define DEBUG 1
//#define TURN_ON_MALLOC_COUNTERS 1

#ifdef DEBUG
#    define DEBUG_PRINT(fmt, ...)                                                                            \
        do {                                                                                                 \
            fprintf(stdout, "#hook_lib: " fmt, ##__VA_ARGS__);                                               \
        } while (0)
#else
#    define DEBUG_PRINT(fmt, ...)
#endif

#ifdef TURN_ON_MALLOC_COUNTERS
#    include <atomic>
std::atomic_int64_t TOTAL_ALLOCS = 0;
std::atomic_int64_t TOTAL_ALLOCATED_BYTES = 0;
#endif

static struct FirstAllocation {
    char          buff[8096];
    unsigned long pos = 0;
    unsigned long allocs = 0;
} first_alloc;

struct BlockFooter {
    std::uintptr_t ret_addr;
    std::size_t    alloc_size;
};

using MallocFunc_t = void* (*)(size_t size);
using CallocFunc_t = void* (*)(size_t elements, size_t size);
using FreeFunc_t = void* (*)(void* ptr);
using ReallocFunc_t = void* (*)(void* ptr, size_t size);
using MemalignFunc_t = void* (*)(size_t blocksize, size_t bytes);
using VallocFunc_t = void* (*)(size_t size);
using PosixMemalignFunc_t = int (*)(void** memptr, size_t alignment, size_t size);

static struct MemoryFunctions {
    MallocFunc_t        malloc;
    CallocFunc_t        calloc;
    FreeFunc_t          free;
    ReallocFunc_t       realloc;
    MemalignFunc_t      memalign;
    VallocFunc_t        valloc;
    PosixMemalignFunc_t posix_memalign;
} mem_func_orig;

// __attribute__((constructor))
static void __lib_hook_init(void) {
    // M_CHECK_ACTION: If bit 0 is set, then print a one-line message on stderr
    // that provides details about the error. Program won't be terminated by
    // calling abort and program will continue execution
    mallopt(M_CHECK_ACTION, 1);
    mem_func_orig.malloc = reinterpret_cast<MallocFunc_t>(dlsym(RTLD_NEXT, "malloc"));
    mem_func_orig.calloc = reinterpret_cast<CallocFunc_t>(dlsym(RTLD_NEXT, "calloc"));
    mem_func_orig.free = reinterpret_cast<FreeFunc_t>(dlsym(RTLD_NEXT, "free"));
    mem_func_orig.realloc = reinterpret_cast<ReallocFunc_t>(dlsym(RTLD_NEXT, "realloc"));
    mem_func_orig.posix_memalign = reinterpret_cast<PosixMemalignFunc_t>(dlsym(RTLD_NEXT, "posix_memalign"));
    mem_func_orig.memalign = reinterpret_cast<MemalignFunc_t>(dlsym(RTLD_NEXT, "memalign"));
    mem_func_orig.valloc = reinterpret_cast<VallocFunc_t>(dlsym(RTLD_NEXT, "valloc"));

    if (!mem_func_orig.malloc || !mem_func_orig.calloc || !mem_func_orig.free || !mem_func_orig.realloc ||
        !mem_func_orig.posix_memalign || !mem_func_orig.memalign || !mem_func_orig.valloc) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
        exit(1);
    }
}

void* try_place_footer(void* ptr, void* ret_addr, size_t size) {
    if (!ptr) {
        return NULL;
    }
    size_t allocatedSize = malloc_usable_size(ptr);
    char*  footerPtr = static_cast<char*>(ptr) + (allocatedSize - sizeof(BlockFooter));
    new (footerPtr) BlockFooter{reinterpret_cast<std::uintptr_t>(ret_addr), size};
    return ptr;
}

static void* malloc_impl(size_t size, void* ret_addr) {
    static int initializing = 0;
    if (mem_func_orig.malloc == NULL) {
        if (!initializing) {
            initializing = 1;
            __lib_hook_init();
            initializing = 0;
        } else if (first_alloc.pos + size < sizeof(first_alloc.buff)) {
            void* ret_ptr = first_alloc.buff + first_alloc.pos;
            first_alloc.pos += size;
            ++first_alloc.allocs;
            return ret_ptr;
        } else {
            fprintf(stderr, "check: too much memory requested during "
                            "initialisation - increase first_alloc.buff size\n");
            exit(1);
        }
    }
#ifdef TURN_ON_MALLOC_COUNTERS
    TOTAL_ALLOCS.fetch_add(1, std::memory_order_relaxed);
    TOTAL_ALLOCATED_BYTES.fetch_add(size, std::memory_order_relaxed);
#endif
    void* dataPtr = mem_func_orig.malloc(size + sizeof(BlockFooter));
    return try_place_footer(dataPtr, ret_addr, size);
}

extern "C" {
void* memset(void*, int, size_t);

void* malloc(size_t size) {
    auto  ret_addr = __builtin_return_address(0);
    void* ptr = malloc_impl(size, ret_addr);
    return ptr;
}

void free(void* ptr) {
    assert(mem_func_orig.malloc != NULL);
    if (!ptr) {
        return;
    }
    if (ptr >= reinterpret_cast<void*>(first_alloc.buff) &&
        ptr <= reinterpret_cast<void*>(first_alloc.buff + first_alloc.pos)) {
        DEBUG_PRINT("free. first_alloc.buf\n");
        return;
    }
#ifdef TURN_ON_MALLOC_COUNTERS
    TOTAL_ALLOCS.fetch_sub(1, std::memory_order_relaxed);
    size_t       allocatedSize = malloc_usable_size(ptr);
    BlockFooter* footerPtr =
        reinterpret_cast<BlockFooter*>(static_cast<char*>(ptr) + (allocatedSize - sizeof(BlockFooter)));
    TOTAL_ALLOCATED_BYTES.fetch_sub(footerPtr->alloc_size, std::memory_order_relaxed);
#endif
    mem_func_orig.free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    DEBUG_PRINT("calloc\n");
    auto  ret_addr = __builtin_return_address(0);
    void* ptr = malloc_impl(nmemb * size, ret_addr);
    if (ptr) {
        memset(ptr, 0, nmemb * size);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    auto ret_addr = __builtin_return_address(0);
    if (!ptr) {
        return malloc_impl(size, ret_addr);
    }
    void* dataPtr = mem_func_orig.realloc(ptr, size + sizeof(BlockFooter));
    return try_place_footer(dataPtr, ret_addr, size);
}

void* memalign(size_t blocksize, size_t bytes) {
    auto ret_addr = __builtin_return_address(0);
    auto ptr = mem_func_orig.memalign(blocksize, bytes);
    return try_place_footer(ptr, ret_addr, bytes);
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    auto ret_addr = __builtin_return_address(0);
    auto rc = mem_func_orig.posix_memalign(memptr, alignment, size + sizeof(BlockFooter));
    try_place_footer(*memptr, ret_addr, size);
    return rc;
}

void* valloc(size_t size) {
    auto ret_addr = __builtin_return_address(0);
    auto ptr = mem_func_orig.valloc(size);
    return try_place_footer(ptr, ret_addr, size);
}
} // extern "C"

void* operator new(size_t sz) {
    DEBUG_PRINT("new(size_t), size = %zu\n", sz);
    if (sz == 0) {
        ++sz; // avoid std::malloc(0) which may return nullptr on success
    }
    auto ret_addr = __builtin_return_address(0);
    if (void* ptr = malloc_impl(sz, ret_addr)) {
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t sz) {
    DEBUG_PRINT("new[](size_t), size = %zu\n", sz);
    if (sz == 0) {
        ++sz; // avoid std::malloc(0) which may return nullptr on success
    }
    auto ret_addr = __builtin_return_address(0);
    if (void* ptr = malloc_impl(sz, ret_addr)) {
        return ptr;
    }
    throw std::bad_alloc{};
}