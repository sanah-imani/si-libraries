# SI Arena

An [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) library for creating [memory arenas](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

### Inspired By
- [STB Libraries](https://github.com/nothings/stb)
- [Sokol Libraries](https://github.com/floooh/sokol)
- [Metadesk](https://github.com/Dion-Systems/metadesk)

**Many parts of the implementation are based on the implementation found in Metadesk**

First off, if you are not already familiar with memory arenas, I recommend reading [this](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator) article to know what they are and why they are useful.

**TL;DR**: Arenas are strictly linear allocators that can be faster and easier to work with than the traditional `malloc` and `free` design pattern.

## Documentation

- [Example](#example)
- [Introduction](#introduction)
- [Typedefs](#typedefs)
- [Enums](#enums)
- [Macros](#macros)
- [Structs](#structs)
- [Functions](#functions)
- [Definitions and Options](#definitions-and-options)
- [Error Handling](#error-handling)
- [Platforms](#platforms)
- [Profiling](#profiling)
- [Memory Pools](#memory-pools)

Backends
--------

`si_arena` uses two different backends depending on the requirements of the application. There is a backend that uses `malloc` and `free`, and there is a backend that uses lower level functions like `VirtualAlloc` and `mmap`.

**NOTE: I recomend using the lower level one, unless you have a good reason not to.**

Example
-------
```c
#include <stdio.h>

// You should put the implementation in a separate source file in a real project
#define SI_ARENA_IMPL
#include "si_arena.h"

void arena_error(sia_error err) {
    fprintf(stderr, "SIA Error %d: %s\n", err.code, err.msg);
}

int main() {
    si_arena* arena = sia_create(&(sia_desc){
        .desired_max_size = SIA_MiB(4),
        .desired_block_size = SIA_KiB(256),
        .error_callback = arena_error
    });

    int* data = (int*)sia_push(arena, sizeof(int) * 64);
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    printf("[ ");
    for (int i = 0; i < 64; i++) {
        printf("%d, ", data[i]);
    }
    printf("\b\b ]\n");

    sia_destroy(arena);

    return 0;
}
```

Introduction
------------

Download the file `si_arena.h`. Create a source file for the implementation. Add the following:
```c
#define SI_ARENA_IMPL
#include "si_arena.h"
```

Create an arena by calling `sia_create`, which takes a pointer to a `sia_desc` structure:
```c
si_arena* arena = sia_create(&(sia_desc){
    .desired_max_size = SIA_MiB(16),
    .desired_block_size = SIA_KiB(256)
});
```
You are required to fill in `desired_max_size`, but all other values will be given defaults by `sia_create`.

Allocate memory by calling `sia_push`:
```c
some_obj* obj = (some_obj*)sia_push(arena, sizeof(some_obj));
```
You can also call `sia_push_zero` to initialize the memory to zero. There are also four macros to make object creation easier:
```c
some_obj* zeroed_obj = (some_obj*)sia_push_zero(arena, sizeof(some_obj));

other_obj* other = SIA_PUSH_STRUCT(arena, other_obj);
other_obj* zeroed_other = SIA_PUSH_ZERO_STRUCT(arena, other_obj);
int* array = SIA_PUSH_ARRAY(arena, int, 64);
int* zeroed_array = SIA_PUSH_ZERO_ARRAY(arena, int, 64);
```

Deallocate memory with `sia_pop` or `sia_pop_to`: 
```c
sia_u64 start_pos = sia_get_pos(arena);
int* arr = SIA_PUSH_ARRAY(arena, int, 64);
// Do something with arr, without allocating any more memory
sia_pop_to(start_pos);
```
**WARNING: Because of memory alignment, `sia_pop` might not deallocate all the memory that you intended it to. It is better to use `sia_pop_to` or temporary arenas (see below).**

Make temporary arenas:
```c
sia_temp temp = sia_temp_begin(arena);

int* data = (int*)sia_push(temp.arena, sizeof(int) * 16);

sia_temp_end(temp);
// data gets deallocated
```

Get temporary memory with scratch arenas:
```c
sia_temp scratch = sia_scratch_get(NULL, 0);

int* data = (int*)sia_push(scratch.arena, sizeof(int) * 64);

sia_scratch_release(scratch);
```

Reset/clear arenas with `sia_reset`:
```c
char* str = (char*)sia_push(arena, sizeof(char) * 10);

sia_reset(arena);
// str gets deallocated
```

Delete arenas with `sia_destroy`:
```c
si_arena* arena = sia_create(&(sia_desc){
    .desired_max_size = SIA_MiB(16),
    .desired_block_size = SIA_KiB(256)
});

// Do stuff with arena

sia_destroy(arena);
```


Typedefs
--------
- `sia_i32`
    - 32 bit signed integer
- `sia_u8`
    - 8 bit unsigned integer
- `sia_u32`
    - 32 bit unsigned integer
- `sia_u64`
    - 64 bit unsigned integer
- `sia_b32`
    - 32 bit boolean
- `sia_error_callback(sia_error error)`
    - Callback function type for errors

Enums
-----
- `sia_error_code`
    - SIA_ERR_NONE
        - No error
    - SIA_ERR_INIT_FAILED
        - Arena failed to init
    - SIA_ERR_MALLOC_FAILED
        - Arena failed to allocate memory with malloc
    - SIA_ERR_COMMIT_FAILED
        - Arena failed to commit memory
    - SIA_ERR_OUT_OF_MEMORY
        - Arena position exceeded arena size
    - SIA_ERR_CANNOT_POP_MORE
        - Arena cannot deallocate any more memory

Macros
------
- `SIA_KiB(x)`
    - Number of bytes per `x` kibibytes (1,024)
- `SIA_MiB(x)`
    - Number of bytes per `x` mebibytes (1,048,576)
- `SIA_GiB(x)`
    - Number of bytes per `x` gibibytes (1,073,741,824)
    
- `SIA_PUSH_STRUCT(arena, type)`
    - Pushes a struct `type` onto `arena`
- `SIA_PUSH_ZERO_STRUCT(arena, type)`
    - Pushes a struct `type` onto `arena` and zeros the memory
- `SIA_PUSH_ARRAY(arena, type, num)`
    - Pushes `num` `type` structs onto `arena`
- `SIA_PUSH_ZERO_ARRAY(arena, type, num)`
    - Pushes `num` `type` structs onto `arena` and zeros the memory

Structs
-------
- `si_arena` - A memory arena
    - `sia_error_callback*` *error_callback*
        - Error callback function (See `sia_error_callback` for more detail)
    - *(all other properties should only be accessed through the getter functions below)*
- `sia_error` - An error
    - `sia_error_code` *code*
        - Error code (see `sia_error_code` for more detail)
    - `char*` *msg*
        - Error message as a c string
- `sia_desc` - initialization parameters for `sia_create`
    - This struct should be made with designated initializer. All uninitialized values (except for *desired_max_size*) will be given defaults.
    - `sia_u64` *desired_max_size*
        - Maximum size of arena, rounded up to nearest page size
    - `sia_u32` *desired_block_size*
        - Desired size of block in arena. The real block size will be rounded to the nearest page size, then reounded to the nearest power of 2. For the malloc backend, a node will be a multiple of the block size. For the lower level backend, memory is committed in multiples of the block size. (See [Backends](#backends))
    - `sia_u32` *align*
        - Size of memory alignment (See [this article](https://developer.ibm.com/articles/pa-dalign/) for rationality) to apply, **Must be power of 2**. To disable alignment, you can pass in a value of 1.
    - `sia_error_callback*` *error_callback*
        - Error callback function (See `sia_error_callback` for more detail)
- `sia_temp` - A temporary arena
    - `si_arena*` arena
        - The `si_arena` object assosiated with the temporary arena


Functions
---------
- `si_arena* sia_create(const sia_desc* desc)` <br>
    - Creates a new `si_arena` according to the sia_desc object.
    - Returns NULL on failure, get the error with the callback function or with `sia_get_error`
- `void sia_destroy(si_arena* arena)` <br>
    - Destroys an `si_arena` object.
- `sia_error sia_get_error(si_arena* arena)` <br>
    - Gets the last error from the given arena. **Arena can be NULL.** If the arena is null, it will give the last error according to a static, thread local variable in the implementation.
- `sia_u64 sia_get_pos(si_arena* arena)`
- `sia_u64 sia_get_size(si_arena* arena)`
- `sia_u32 sia_get_block_size(si_arena* arena)`
- `sia_u32 sia_get_align(si_arena* arena)`
    - (See `sia_desc` for more detail about what these mean)
- `void* sia_push(si_arena* arena, sia_u64 size)`
    - Allocates `size` bytes on the arena.
    - Retruns NULL on failure
- `void* sia_push_zero(si_arena* arena, sia_u64 size)`
    - Allocates `size` bytes on the arena and zeros the memory.
    - Returns NULL on failure
- `void sia_pop(si_arena* arena, sia_u64 size)`
    - Pops `size` bytes from the arena.
    - **WARNING: Because of memory alignment, this may not always act as expected. Make sure you know what you are doing.**
    - Fails if you attempt to pop too much memory
- `void sia_pop_to(si_arena* arena, sia_u64 pos)`
    - Pops memory from the arena, setting the arenas position to `pos`.
    - **WARNING: Because of memory alignment, this may not always act as expected. Make sure you know what you are doing.**
    - Fails if you attempt to pop too much memory
- `void sia_reset(si_arena* arena)`
    - Deallocates all memory in arena, returning the arena to its original position.
    - NOTE: Always use `sia_reset` instead of `sia_pop_to` if you need to clear all memory. Position 0 is not always the start of the arena. 
- `sia_temp sia_temp_begin(si_arena* arena)`
    - Creates a new temporary arena from the given arena.
- `void sia_temp_end(sia_temp temp)`
    - Destroys the temporary arena, deallocating all allocations made with the temporary arena.
- `void sia_scratch_set_desc(const sia_desc* desc)`
    - Sets the `sia_desc` used to initialize scratch arenas.
    - NOTE: This will only work before any calls to `sia_scratch_get`
    - The default desc has a `desired_max_size` of 64 MiB and a `desired_block_size` of 128 KiB
- `sia_temp sia_scratch_get(si_arena** conflicts, sia_u32 num_conflicts)`
    - Gets a thread local scratch arena
    - You can pass in a list of conflict scratch arenas. One example where this is useful is if you have a function that gets a scratch arena calling another function that gets another scratch arena:
        - ```c
          int* func_b(si_arena* arena) {
              sia_temp scratch = sia_scratch_get(&arena, 1);
              // Do stuff
              sia_scratch_release(scratch);
          }
          void func_a() {
              sia_temp scratch = sia_scratch_get(NULL, 0);
              func_b(scratch);
              sia_scratch_release(scratch);
          }
- `void sia_scratch_release(sia_temp scratch)`
    - Releases the scratch arena

- `void* sia_realloc(si_arena* arena, void* ptr, sia_u64 old_size, sia_u64 new_size)` <br>
    - *(Planned Feature)* Reallocates memory previously allocated with `sia_push` or `sia_push_zero`.
    - Attempts to grow the allocation in-place if there is space available after the pointer.
    - If in-place growth is not possible, allocates new memory and copies the old data.
    - Returns the new pointer (which may be the same as `ptr` if in-place growth succeeded).
    - Returns NULL on failure.
    - **WARNING**: The old pointer becomes invalid if reallocation moves the data. Always use the returned pointer.
    - Example:
        ```c
        int* arr = SIA_PUSH_ARRAY(arena, int, 10);
        // Need more space
        arr = (int*)sia_realloc(arena, arr, sizeof(int) * 10, sizeof(int) * 20);
        ```

- `si_arena* sia_merge(si_arena** arenas, sia_u32 num_arenas)` <br>
    - *(Planned Feature)* Merges multiple arenas into a single new arena.
    - All memory from the source arenas is copied into the new arena.
    - The new arena's size is the sum of all used memory from the source arenas, rounded up appropriately.
    - Returns a new `si_arena` on success, NULL on failure.
    - **NOTE**: The source arenas are not destroyed by this function. You must call `sia_destroy` on them separately if desired.
    - **NOTE**: Pointers from the original arenas become invalid. This is primarily useful for consolidating memory or creating snapshots.
    - Example:
        ```c
        si_arena* arena1 = sia_create(&(sia_desc){ .desired_max_size = SIA_MiB(4) });
        si_arena* arena2 = sia_create(&(sia_desc){ .desired_max_size = SIA_MiB(4) });
        
        // Allocate in both arenas
        int* data1 = SIA_PUSH_ARRAY(arena1, int, 100);
        int* data2 = SIA_PUSH_ARRAY(arena2, int, 100);
        
        // Merge them
        si_arena* arenas[] = { arena1, arena2 };
        si_arena* merged = sia_merge(arenas, 2);
        
        // Clean up original arenas
        sia_destroy(arena1);
        sia_destroy(arena2);
        ```

Definitions and Options
-----------------------

Define these above where you put the implementation. Example:
```c
#define SIA_FORCE_MALLOC
#define SIA_MALLOC custom_malloc
#define SIA_FREE custom_free
#define SI_ARENA_IMPL
#include "si_arena.h"
```

- `SIA_FORCE_MALLOC`
    - Enables the `malloc` based backend
- `SIA_MALLOC` and `SIA_FREE`
    - If you are using the malloc backend (because of an unknown platform or `SIA_FORCE_MALLOC`), you can provide your own implementations of `malloc` and `free` to avoid the c standard library.
- `SIA_MEMSET`
    - Provide a custom implementation of `memset` to avoid the c standard library.
- `SIA_THREAD_VAR`
    - Provide the implementation for creating a thread local variable if it is not supported.
- `SIA_FUNC_DEF`
    - Add custom prefix to all functions
- `SIA_STATIC`
    - Makes all functions static.
- `SIA_DLL`
    - Adds `__declspec(dllexport)` or `__declspec(dllimport)` to all functions.
    - NOTE: `SIA_STATIC` and `SIA_DLL` do not work simultaneously and they do not work if you have defined `SIA_FNC_DEF`.
- `SIA_SCRATCH_COUNT`
    - Number of scratch arenas per thread
    - Default is 2
- `SIA_MEM_RESERVE` and related
    - See [Platforms](#platforms)
- `SIA_ENABLE_PROFILING`
    - *(Planned Feature)* Enables performance profiling hooks for arena operations.
    - When enabled, allows registration of a profile callback to track allocation/deallocation performance.
    - Default is disabled (0).
- `SIA_PROFILE_CALLBACK`
    - *(Planned Feature)* Define a custom profile callback function type.
    - If not defined, uses the default `sia_profile_callback` type.

Error Handling
--------------
There are two ways to do error handling, you can use both or neither. **Errors will not be displayed by default.** <br>
An error has a code (`sia_error_code` enum) and a c string (`char*`) message;

- Callback functions
    - The first way is to register a callback function. The callback function is unique to the arena, so you can mix and match if you like.
     ```c
    #include <stdio.h>

    // In a real application, the implementaion should be in another c file
    #define SI_ARENA_IMPL
    #include "si_arena.h"

    void error_callback(sia_error error) {
        fprintf(stderr, "SIA Error %d: %s\n", error.code, error.msg);
    }

    int main() {
        si_arena* arena = sia_create(&(sia_desc){
            .desired_max_size = SIA_MiB(4),
            .error_callback = error_callback
        });

        sia_push(arena, SIA_MiB(5));
        // error_callback gets called

        sia_destroy(arena);

        return 0;
    }
     ```
- `sia_error sia_get_error(si_arena* arena)`
    - This is another way to get the error struct.
    - This function does work with a NULL pointer in case the arena is not initialized correctly. If the pointer given to the function is NULL, the function uses a thead local and static variable.
    - I would recommend using the callback because it will always be assosiated with the arena.
    ```c
    #include <stdio.h>

    // In a real application, the implementaion should be in another c file
    #define SI_ARENA_IMPL
    #include "si_arena.h"

    int main() {
        si_arena* arena = sia_create(&(sia_desc){
            .desired_max_size = SIA_MiB(4),
        });

        int* data = (int*)sia_push(arena, SIA_MiB(5));
        if (data == NULL) {
            sia_error error = sia_get_error(arena);
            fprintf(stderr, "SIA Error %d: %s\n", error.code, error.msg);
        }

        sia_destroy(arena);

        return 0;
    }
    ```

Platforms
---------
Here is a list of the platforms that are currently supported:
- Windows
- Linux
- MacOS
- Emscripten

Using the low level backend is always prefered by si_arena, but the malloc backend is used if it is not possible or the platform is unknown. If you are using a platform the is unsupported **and** do not want to use the malloc backend, you can do the following:

(For this example, I will show how you could implement this for Windows, even though you would not actually have to do this for Windows).

To use the low level backend for an unknown platform, you have to create five functions and set corresponding definitions. I would recommend using `<stdint.h>` or something similar for these functions.
```c
// Reserves size bytes
// Returns pointer to data
void* mem_reserve(uint64_t size);
// Commits size bytes, starting at ptr
// Returns 1 if the commit worked, 0 on failure
int32_t mem_commit(void* ptr, uint64_t size);
// Decommits size bytes, starting at ptr
void mem_decommit(void* ptr, uint64_t size);
// Releases size bytes, starting at ptr
void mem_release(void* ptr, uint64_t size);
// Gets the page size of the system
uint32_t mem_pagesize();

#define SIA_MEM_RESERVE mem_reserve
#define SIA_MEM_COMMIT mem_commit
#define SIA_MEM_DECOMMIT mem_decommit
#define SIA_MEM_RELEASE mem_release
#define SIA_MEM_PAGESIZE mem_pagesize

#define SI_ARENA_IMPL
#include "si_arena.h"
```

Here is what it would look like on Windows:
```c
#include <stdint.h>
#include <Windows.h>

void* win32_mem_reserve(uint64_t size) {
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}
int32_t win32_mem_commit(void* ptr, uint64_t size) {
    return (int32_t)(VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
}
void win32_mem_decommit(void* ptr, uint64_t size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}
void win32_mem_release(void* ptr, uint64_t size) {
    // size is unused
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}
uint32_t win32_mem_pagesize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (uint32_t)si.dwPageSize;
}

#define SIA_MEM_RESERVE win32_mem_reserve
#define SIA_MEM_COMMIT win32_mem_commit
#define SIA_MEM_DECOMMIT win32_mem_decommit
#define SIA_MEM_RELEASE win32_mem_release
#define SIA_MEM_PAGESIZE win32_mem_pagesize

#define SI_ARENA_IMPL
#include "si_arena.h"
```

Profiling
---------
*(Planned Feature)*

The profiling system allows you to track performance metrics for arena operations. This is useful for identifying memory allocation hotspots and optimizing your memory usage patterns.

To enable profiling, define `SIA_ENABLE_PROFILING` before including the implementation:
```c
#define SIA_ENABLE_PROFILING
#define SI_ARENA_IMPL
#include "si_arena.h"
```

### Profile Callback

Register a profile callback to receive performance data:
```c
typedef struct {
    const char* operation;  // "push", "pop", "realloc", etc.
    sia_u64 size;          // Size in bytes
    sia_u64 time_ns;       // Time taken in nanoseconds
    si_arena* arena;       // Arena that performed the operation
} sia_profile_event;

typedef void (*sia_profile_callback)(sia_profile_event event);

void my_profile_callback(sia_profile_event event) {
    printf("Operation: %s, Size: %llu bytes, Time: %llu ns\n", 
           event.operation, event.size, event.time_ns);
}

int main() {
    sia_set_profile_callback(my_profile_callback);
    
    si_arena* arena = sia_create(&(sia_desc){
        .desired_max_size = SIA_MiB(4)
    });
    
    // All operations will now be profiled
    int* data = SIA_PUSH_ARRAY(arena, int, 1000);
    
    sia_destroy(arena);
    return 0;
}
```

### Profile Functions

- `void sia_set_profile_callback(sia_profile_callback callback)` <br>
    - Sets the global profile callback function.
    - Pass NULL to disable profiling.
    - The callback will be called for all arena operations when profiling is enabled.

- `sia_profile_stats sia_get_profile_stats(si_arena* arena)` <br>
    - Gets accumulated profile statistics for a specific arena.
    - Returns a struct containing total operations, total time, peak allocation rate, etc.
    - Only available when `SIA_ENABLE_PROFILING` is defined.

### Profile Statistics

```c
typedef struct {
    sia_u64 total_push_operations;
    sia_u64 total_pop_operations;
    sia_u64 total_push_bytes;
    sia_u64 total_pop_bytes;
    sia_u64 total_time_ns;
    sia_u64 peak_allocation_rate;  // Bytes per second
} sia_profile_stats;
```

Memory Pools
------------
*(Planned Feature)*

Fixed-size block allocators built on top of arenas. Pools allow random-order allocation and deallocation of same-sized blocks with automatic reuse.

### Pool Functions

- `sia_pool* sia_pool_create(const sia_pool_desc* desc)` <br>
    - Creates a new memory pool with fixed-size blocks.
    - Returns NULL on failure.

- `void sia_pool_destroy(sia_pool* pool)` <br>
    - Destroys a memory pool. Backing arena is not destroyed.

- `void* sia_pool_alloc(sia_pool* pool)` <br>
    - Allocates one block from the pool. Returns NULL on failure.

- `void* sia_pool_alloc_zero(sia_pool* pool)` <br>
    - Allocates one zeroed block from the pool. Returns NULL on failure.

- `void sia_pool_free(sia_pool* pool, void* ptr)` <br>
    - Returns a block to the pool for reuse. Pointer must be from this pool.

- `sia_b32 sia_pool_grow(sia_pool* pool, sia_u64 num_blocks)` <br>
    - Grows pool capacity by allocating additional blocks. Returns SIA_TRUE on success.

- `sia_u64 sia_pool_get_block_size(sia_pool* pool)` <br>
    - Returns the fixed block size.

- `sia_u64 sia_pool_get_capacity(sia_pool* pool)` <br>
    - Returns total number of blocks the pool can hold.

- `sia_u64 sia_pool_get_used(sia_pool* pool)` <br>
    - Returns number of currently allocated blocks.

- `sia_u64 sia_pool_get_free(sia_pool* pool)` <br>
    - Returns number of blocks in the free list.

### Pool Structs

- `sia_pool` - A memory pool (internal structure)

- `sia_pool_desc` - Pool initialization parameters
    - `si_arena*` *arena* - Backing arena for the pool
    - `sia_u64` *block_size* - Fixed size of each block (must be >= sizeof(void*))
    - `sia_u32` *align* - Block alignment (must be power of 2, defaults to block_size)
    - `sia_u64` *initial_capacity* - Initial number of blocks to allocate

### Pool Macros

- `SIA_POOL_ALLOC_STRUCT(pool, type)` - Allocates one block cast to `type*`
- `SIA_POOL_ALLOC_ZERO_STRUCT(pool, type)` - Allocates one zeroed block cast to `type*`

### Pool Error Codes

- `SIA_ERR_POOL_FULL` - Pool has no free blocks and cannot grow
- `SIA_ERR_INVALID_POOL_PTR` - Attempted to free invalid pointer

### TODO
- Article about implementation
- Implement realloc feature
- Implement arena merge feature
- Implement profiling feature
- Implement memory pools feature
