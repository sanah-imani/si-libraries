#ifndef SI_ARENA_H
#define SI_ARENA_H

#ifndef SIA_FUNC_DEF
#   if defined(SIA_STATIC)
#      define SIA_FUNC_DEF static
#   elif defined(_WIN32) && defined(SIA_DLL) && defined(SI_ARENA_IMPL)
#       define SIA_FUNC_DEF __declspec(dllexport)
#   elif defined(_WIN32) && defined(SIA_DLL)
#       define SIA_FUNC_DEF __declspec(dllimport)
#   else
#      define SIA_FUNC_DEF extern
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int32_t  sia_i32;
typedef uint8_t  sia_u8;
typedef uint32_t sia_u32;
typedef uint64_t sia_u64;

typedef sia_i32 sia_b32;

#define SIA_KiB(x) (sia_u64)((sia_u64)(x) << 10)
#define SIA_MiB(x) (sia_u64)((sia_u64)(x) << 20)
#define SIA_GiB(x) (sia_u64)((sia_u64)(x) << 30) 

typedef struct _sia_malloc_node {
    struct _sia_malloc_node* prev;
    sia_u64 size;
    sia_u64 pos;
    sia_u8* data;
} _sia_malloc_node;

typedef struct {
    _sia_malloc_node* cur_node;
} _sia_malloc_backend;
typedef struct {
    sia_u64 commit_pos;
} _sia_reserve_backend;

typedef enum {
    SIA_ERR_NONE = 0,
    SIA_ERR_INIT_FAILED,
    SIA_ERR_MALLOC_FAILED,
    SIA_ERR_COMMIT_FAILED,
    SIA_ERR_OUT_OF_MEMORY,
    SIA_ERR_CANNOT_POP_MORE,
    SIA_ERR_REALLOC_FAILED,
    SIA_ERR_INVALID_PTR,
    SIA_ERR_MERGE_FAILED,
} sia_error_code;

typedef struct {
    sia_error_code code;
    char* msg;
} sia_error;

typedef void (sia_error_callback)(sia_error error);


typedef struct {
    sia_u64 _pos;

    sia_u64 _size;
    sia_u64 _block_size;
    sia_u32 _align;

    union {
        _sia_malloc_backend _malloc_backend;
        _sia_reserve_backend _reserve_backend;
    };

    sia_error _last_error;
    sia_error_callback* error_callback;
} si_arena;

typedef struct {
    sia_u64 desired_max_size;
    sia_u32 desired_block_size;
    sia_u32 align;
    sia_error_callback* error_callback;
} sia_desc;

SIA_FUNC_DEF si_arena* sia_create(const sia_desc* desc);
SIA_FUNC_DEF void sia_destroy(si_arena* arena);

SIA_FUNC_DEF sia_error sia_get_error(si_arena* arena);

SIA_FUNC_DEF sia_u64 sia_get_pos(si_arena* arena);
SIA_FUNC_DEF sia_u64 sia_get_size(si_arena* arena);
SIA_FUNC_DEF sia_u32 sia_get_block_size(si_arena* arena);
SIA_FUNC_DEF sia_u32 sia_get_align(si_arena* arena);

SIA_FUNC_DEF void* sia_push(si_arena* arena, sia_u64 size);
SIA_FUNC_DEF void* sia_push_zero(si_arena* arena, sia_u64 size);
SIA_FUNC_DEF void* sia_realloc(si_arena* arena, void* ptr, sia_u64 old_size, sia_u64 new_size);

SIA_FUNC_DEF void sia_pop(si_arena* arena, sia_u64 size);
SIA_FUNC_DEF void sia_pop_to(si_arena* arena, sia_u64 pos);

SIA_FUNC_DEF void sia_reset(si_arena* arena);

#define SIA_PUSH_STRUCT(arena, type) (type*)sia_push(arena, sizeof(type))
#define SIA_PUSH_ZERO_STRUCT(arena, type) (type*)sia_push_zero(arena, sizeof(type))
#define SIA_PUSH_ARRAY(arena, type, num) (type*)sia_push(arena, sizeof(type) * (num))
#define SIA_PUSH_ZERO_ARRAY(arena, type, num) (type*)sia_push_zero(arena, sizeof(type) * (num))

typedef struct {
    si_arena* arena;
    sia_u64 _pos;
} sia_temp;

SIA_FUNC_DEF sia_temp sia_temp_begin(si_arena* arena);
SIA_FUNC_DEF void sia_temp_end(sia_temp temp);

SIA_FUNC_DEF void sia_scratch_set_desc(const sia_desc* desc);
SIA_FUNC_DEF sia_temp sia_scratch_get(si_arena** conflicts, sia_u32 num_conflicts);
SIA_FUNC_DEF void sia_scratch_release(sia_temp scratch);

SIA_FUNC_DEF si_arena*  sia_merge(si_arena** arenas, sia_u32 num_arenas);

#ifdef __cplusplus
}
#endif

#endif // SI_ARENA_H

/*
SIA Implementation
===========================================================================================
  __  __  ___   _     ___ __  __ ___ _    ___ __  __ ___ _  _ _____ _ _____ ___ ___  _  _ 
 |  \/  |/ __| /_\   |_ _|  \/  | _ \ |  | __|  \/  | __| \| |_   _/_\_   _|_ _/ _ \| \| |
 | |\/| | (_ |/ _ \   | || |\/| |  _/ |__| _|| |\/| | _|| .` | | |/ _ \| |  | | (_) | .` |
 |_|  |_|\___/_/ \_\ |___|_|  |_|_| |____|___|_|  |_|___|_|\_| |_/_/ \_\_| |___\___/|_|\_|

===========================================================================================
*/

#ifdef SI_ARENA_IMPL

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)
#    define SIA_PLATFORM_LINUX
#elif defined(__APPLE__)
#    define SIA_PLATFORM_APPLE
#elif defined(_WIN32)
#    define SIA_PLATFORM_WIN32
#elif defined(__EMSCRIPTEN__)
#    define SIA_PLATFORM_EMSCRIPTEN
#else
#    warning "SIA: Unknown platform"
#    define SIA_PLATFORM_UNKNOWN
#endif

#if defined(SIA_MEM_RESERVE) && defined(SIA_MEM_COMMIT) && defined(SIA_MEM_DECOMMIT) && defined(SIA_MEM_RELEASE) && defined(SIA_MEM_PAGESIZE)
#elif !defined(SIA_MEM_RESERVE) && !defined(SIA_MEM_COMMIT) && !defined(SIA_MEM_DECOMMIT) && !defined(SIA_MEM_RELEASE) && !defined(SIA_MEM_PAGESIZE)
#else
#    error "SI ARENA: Must define all or none of, SIA_MEM_RESERVE, SIA_MEM_COMMIT, SIA_MEM_DECOMMIT, SIA_MEM_RELEASE, and SIA_MEM_PAGESIZE"
#endif

#if !defined(SIA_MEM_RESERVE) && !defined(SIA_FORCE_MALLOC) && (defined(SIA_PLATFORM_LINUX) || defined(SIA_PLATFORM_WIN32))
#    define SIA_MEM_RESERVE _sia_mem_reserve
#    define SIA_MEM_COMMIT _sia_mem_commit
#    define SIA_MEM_DECOMMIT _sia_mem_decommit
#    define SIA_MEM_RELEASE _sia_mem_release
#    define SIA_MEM_PAGESIZE _sia_mem_pagesize
#endif

// This is needed for the size and block_size calculations
#ifndef SIA_MEM_PAGESIZE
#    define SIA_MEM_PAGESIZE _sia_mem_pagesize
#endif

#if !defined(SIA_MEM_RESERVE) && !defined(SIA_FORCE_MALLOC)
#   define SIA_FORCE_MALLOC
#endif

#if defined(SIA_FORCE_MALLOC)
#    if defined(SIA_MALLOC) && defined(SIA_FREE)
#    elif !defined(SIA_MALLOC) && !defined(SIA_FREE)
#    else
#        error "SIA ARENA: Must define both or none of SIA_MALLOC and SIA_FREE"
#    endif
#    ifndef SIA_MALLOC
#        include <stdlib.h>
#        define SIA_MALLOC malloc
#        define SIA_FREE free
#    endif
#endif

#ifndef SIA_MEMSET
#   include <string.h>
#   define SIA_MEMSET memset
#endif

#ifndef SIA_MEMCPY
#   include <string.h>
#   define SIA_MEMCPY memcpy
#endif

#ifndef SIA_NO_STDIO
#   include <stdio.h>
#endif

#define SIA_UNUSED(x) (void)(x)

#define SIA_TRUE 1
#define SIA_FALSE 0

#ifndef SIA_THREAD_VAR
#    if defined(__clang__) || defined(__GNUC__)
#        define SIA_THREAD_VAR __thread
#    elif defined(_MSC_VER)
#        define SIA_THREAD_VAR __declspec(thread)
#    elif (__STDC_VERSION__ >= 201112L)
#        define SIA_THREAD_VAR _Thread_local
#    else
#        error "SI ARENA: Invalid compiler/version for thead variable; Define SIA_THREAD_VAR, use Clang, GCC, or MSVC, or use C11 or greater"
#    endif
#endif

#define SIA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIA_MAX(a, b) ((a) > (b) ? (a) : (b))

#define SIA_ALIGN_UP_POW2(x, b) (((sia_u64)(x) + ((sia_u64)(b) - 1)) & (~((sia_u64)(b) - 1)))

#ifdef SIA_PLATFORM_WIN32

#ifndef UNICODE
    #define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#ifndef SIA_FORCE_MALLOC
static void* _sia_mem_reserve(sia_u64 size) {
    void* out = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return out;
}
static sia_b32 _sia_mem_commit(void* ptr, sia_u64 size) {
    sia_b32 out = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return out;
}
static void _sia_mem_decommit(void* ptr, sia_u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}
static void _sia_mem_release(void* ptr, sia_u64 size) {
    SIA_UNUSED(size);
    VirtualFree(ptr, 0, MEM_RELEASE);
}
#endif
static sia_u32 _sia_mem_pagesize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (sia_u32)si.dwPageSize;
}

#endif // SIA_PLATFORM_WIN32

#if defined(SIA_PLATFORM_LINUX) || defined(SIA_PLATFORM_APPLE)

#include <sys/mman.h>
#include <unistd.h>

#ifndef SIA_FORCE_MALLOC
static void* _sia_mem_reserve(sia_u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
    return out;
}
static sia_b32 _sia_mem_commit(void* ptr, sia_u64 size) {
    sia_b32 out = (mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0);
    return out;
}
static void _sia_mem_decommit(void* ptr, sia_u64 size) {
    mprotect(ptr, size, PROT_NONE);
    madvise(ptr, size, MADV_DONTNEED);
}
static void _sia_mem_release(void* ptr, sia_u64 size) {
    munmap(ptr, size);
}
#endif
static sia_u32 _sia_mem_pagesize() {
    return (sia_u32)sysconf(_SC_PAGESIZE);
}

#endif // SIA_PLATFORM_LINUX || SIA_PLATFORM_APPLE

#ifdef SIA_PLATFORM_UNKNOWN

#ifndef SIA_FORCE_MALLOC
static void* _sia_mem_reserve(sia_u64 size) { SIA_UNUSED(size); return NULL; }
static void _sia_mem_commit(void* ptr, sia_u64 size) { SIA_UNUSED(ptr); SIA_UNUSED(size); }
static void _sia_mem_decommit(void* ptr, sia_u64 size) { SIA_UNUSED(ptr); SIA_UNUSED(size); }
static void _sia_mem_release(void* ptr, sia_u64 size) { SIA_UNUSED(ptr); SIA_UNUSED(size); }
#endif
static sia_u32 _sia_mem_pagesize(){ return 4096; }

#endif // SIA_PLATFORM_UNKNOWN

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static sia_u32 _sia_round_pow2(sia_u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    
    return v;
}


typedef struct {
    sia_error_callback* error_callback;
    sia_u64 max_size;
    sia_u32 block_size;
    sia_u32 align;
} _sia_init_data;


static void _sia_empty_error_callback(sia_error error) {
    SIA_UNUSED(error);
}

#ifndef SIA_NO_STDIO
static void _sia_stderr_error_callback(sia_error error) {
    fprintf(stderr, "SIA Error %d: %s\n", error.code, error.msg);
}
#endif


static _sia_init_data _sia_init_common(const sia_desc* desc) {
    _sia_init_data out = { 0 };
    
    out.error_callback = desc->error_callback == NULL ?
        _sia_empty_error_callback : desc->error_callback;

    sia_u32 page_size = SIA_MEM_PAGESIZE();
    
    out.max_size = SIA_ALIGN_UP_POW2(desc->desired_max_size, page_size);
    sia_u32 desired_block_size = desc->desired_block_size == 0 ? 
        SIA_ALIGN_UP_POW2(out.max_size / 8, page_size) : desc->desired_block_size;
    desired_block_size = SIA_ALIGN_UP_POW2(desired_block_size, page_size);
    
    out.block_size = _sia_round_pow2(desired_block_size);
    
    out.align = desc->align == 0 ? (sizeof(void*)) : desc->align;
    
    return out;
}


static SIA_THREAD_VAR sia_error_callback* _sia_global_error_callback = NULL;

// This is an annoying placement, but
// it has to be above the implementations that reference it
static SIA_THREAD_VAR sia_error last_error;

#ifdef SIA_FORCE_MALLOC

/*
Malloc Backend
======================================================================
  __  __   _   _    _    ___   ___   ___   _   ___ _  _____ _  _ ___  
 |  \/  | /_\ | |  | |  / _ \ / __| | _ ) /_\ / __| |/ / __| \| |   \
 | |\/| |/ _ \| |__| |_| (_) | (__  | _ \/ _ \ (__| ' <| _|| .` | |) |
 |_|  |_/_/ \_\____|____\___/ \___| |___/_/ \_\___|_|\_\___|_|\_|___/ 

======================================================================
*/
                                                                      
si_arena* sia_create(const sia_desc* desc) {
    _sia_init_data init_data = _sia_init_common(desc);

    si_arena* out = (si_arena*)malloc(sizeof(si_arena));

    if (out == NULL) {
        last_error.code = SIA_ERR_INIT_FAILED;
        last_error.msg = "Failed to malloc initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }
    
    out->_pos = 0;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->_last_error = (sia_error){ .code=SIA_ERR_NONE, .msg="" };
    out->error_callback = init_data.error_callback;

    out->_malloc_backend.cur_node = (_sia_malloc_node*)malloc(sizeof(_sia_malloc_node));
    *out->_malloc_backend.cur_node = (_sia_malloc_node){
        .prev = NULL,
        .size = out->_block_size,
        .pos = 0,
        .data = (sia_u8*)malloc(out->_block_size)
    };

    return out;
}
void sia_destroy(si_arena* arena) {
    _sia_malloc_node* node = arena->_malloc_backend.cur_node;
    while (node != NULL) {
        free(node->data);

        _sia_malloc_node* temp = node;
        node = node->prev;
        free(temp);
    }
    
    free(arena);
}

void* sia_push(si_arena* arena, sia_u64 size) {
    if (arena->_pos + size > arena->_size) {
        last_error.code = SIA_ERR_OUT_OF_MEMORY;
        last_error.msg = "Arena ran out of memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }

    _sia_malloc_node* node = arena->_malloc_backend.cur_node;

    sia_u64 pos_aligned = SIA_ALIGN_UP_POW2(node->pos, arena->_align);
    sia_u32 diff = pos_aligned - node->pos;
    arena->_pos += diff + size;

    if (arena->_pos >= node->size) {
        
        sia_u64 unclamped_node_size = SIA_ALIGN_UP_POW2(size, arena->_block_size);
        sia_u64 max_node_size = arena->_size - arena->_pos;
        sia_u64 node_size = SIA_MIN(unclamped_node_size, max_node_size);
        
        _sia_malloc_node* new_node = (_sia_malloc_node*)malloc(sizeof(_sia_malloc_node));
        sia_u8* data = (sia_u8*)malloc(node_size);

        if (new_node == NULL || data == NULL) {
            if (new_node != NULL) { free(new_node); }
            if (data != NULL) { free(data); }
            
            last_error.code = SIA_ERR_MALLOC_FAILED;
            last_error.msg = "Failed to malloc new node";
            arena->_last_error = last_error;
            arena->error_callback(last_error);
            return NULL;
        }

        new_node->pos = size;
        new_node->size = node_size;
        new_node->data = data;
        
        new_node->prev = node;
        arena->_malloc_backend.cur_node = new_node;

        return (void*)(new_node->data);
    }
    
    void* out = (void*)((sia_u8*)node->data + pos_aligned);
    node->pos = pos_aligned + size;

    return out;
}

void sia_pop(si_arena* arena, sia_u64 size) {
    if (size > arena->_pos) {
        last_error.code = SIA_ERR_CANNOT_POP_MORE;
        last_error.msg = "Attempted to pop too much memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
    }
    
    sia_u64 size_left = size;
    _sia_malloc_node* node = arena->_malloc_backend.cur_node;

    while (size_left > node->pos) {
        size_left -= node->pos;
        
        _sia_malloc_node* temp = node;
        node = node->prev;

        free(temp->data);
        free(temp);
    }

    arena->_malloc_backend.cur_node = node;

    node->pos -= size_left;
    arena->_pos -= size;
}

void sia_reset(si_arena* arena) {
    sia_pop_to(arena, 0);
}

#else // SIA_FORCE_MALLOC

/*
Low Level Backend
================================================================================
  _    _____      __  _    _____   _____ _      ___   _   ___ _  _____ _  _ ___  
 | |  / _ \ \    / / | |  | __\ \ / / __| |    | _ ) /_\ / __| |/ / __| \| |   \
 | |_| (_) \ \/\/ /  | |__| _| \ V /| _|| |__  | _ \/ _ \ (__| ' <| _|| .` | |) |
 |____\___/ \_/\_/   |____|___| \_/ |___|____| |___/_/ \_\___|_|\_\___|_|\_|___/ 

================================================================================
*/

#define SIA_MIN_POS SIA_ALIGN_UP_POW2(sizeof(si_arena), 64) 

si_arena* sia_create(const sia_desc* desc) {
    _sia_init_data init_data = _sia_init_common(desc);
    
    si_arena* out = SIA_MEM_RESERVE(init_data.max_size);

    if (out == NULL) {
        last_error.code = SIA_ERR_INIT_FAILED;
        last_error.msg = "Failed to reserve initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }

    if (!SIA_MEM_COMMIT(out, init_data.block_size)) {
        last_error.code = SIA_ERR_INIT_FAILED;
        last_error.msg = "Failed to commit initial memory for arena";
        init_data.error_callback(last_error);
        return NULL;
    }

    out->_pos = SIA_MIN_POS;
    out->_size = init_data.max_size;
    out->_block_size = init_data.block_size;
    out->_align = init_data.align;
    out->_reserve_backend.commit_pos = init_data.block_size;
    out->_last_error = (sia_error){ .code=SIA_ERR_NONE, .msg="" };
    out->error_callback = init_data.error_callback;

    return out;
}
void sia_destroy(si_arena* arena) {
    SIA_MEM_RELEASE(arena, arena->_size);
}

void* sia_push(si_arena* arena, sia_u64 size) {
    if (arena->_pos + size > arena->_size) {
        last_error.code = SIA_ERR_OUT_OF_MEMORY;
        last_error.msg = "Arena ran out of memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }

    sia_u64 pos_aligned = SIA_ALIGN_UP_POW2(arena->_pos, arena->_align);
    void* out = (void*)((sia_u8*)arena + pos_aligned);
    arena->_pos = pos_aligned + size;

    sia_u64 commit_pos = arena->_reserve_backend.commit_pos;
    if (arena->_pos > commit_pos) {
        sia_u64 commit_unclamped = SIA_ALIGN_UP_POW2(arena->_pos, arena->_block_size);
        sia_u64 new_commit_pos = SIA_MIN(commit_unclamped, arena->_size);
        sia_u64 commit_size = new_commit_pos - commit_pos;
        
        if (!SIA_MEM_COMMIT((void*)((sia_u8*)arena + commit_pos), commit_size)) {
            last_error.code = SIA_ERR_COMMIT_FAILED;
            last_error.msg = "Failed to commit memory";
            arena->_last_error = last_error;
            arena->error_callback(last_error);
            return NULL;
        }

        arena->_reserve_backend.commit_pos = new_commit_pos;
    }

    return out;
}

void sia_pop(si_arena* arena, sia_u64 size) {
    if (size > arena->_pos - SIA_MIN_POS) {
        last_error.code = SIA_ERR_CANNOT_POP_MORE;
        last_error.msg = "Attempted to pop too much memory";
        arena->_last_error = last_error;
        arena->error_callback(last_error);

        return;
    }

    arena->_pos = SIA_MAX(SIA_MIN_POS, arena->_pos - size);

    sia_u64 new_commit = SIA_MIN(arena->_size, SIA_ALIGN_UP_POW2(arena->_pos, arena->_block_size));
    sia_u64 commit_pos = arena->_reserve_backend.commit_pos;

    if (new_commit < commit_pos) {
        sia_u64 decommit_size = commit_pos - new_commit;
        SIA_MEM_DECOMMIT((void*)((sia_u8*)arena + new_commit), decommit_size);
        arena->_reserve_backend.commit_pos = new_commit;
    }
}

void sia_reset(si_arena* arena) {
    sia_pop_to(arena, SIA_MIN_POS);
}

#endif // NOT SIA_FORCE_MALLOC

/*
All Backends
=========================================================
    _   _    _      ___   _   ___ _  _____ _  _ ___  ___ 
   /_\ | |  | |    | _ ) /_\ / __| |/ / __| \| |   \/ __|
  / _ \| |__| |__  | _ \/ _ \ (__| ' <| _|| .` | |) \__ \
 /_/ \_\____|____| |___/_/ \_\___|_|\_\___|_|\_|___/|___/

=========================================================
*/


sia_error sia_get_error(si_arena* arena) {
    sia_error* err = arena == NULL ? &last_error : &arena->_last_error;
    sia_error temp = *err;

    *err = (sia_error){ SIA_ERR_NONE, "" };
    
    return temp;
}

sia_u64 sia_get_pos(si_arena* arena) { return arena->_pos; }
sia_u64 sia_get_size(si_arena* arena) { return arena->_size; }
sia_u32 sia_get_block_size(si_arena* arena) { return arena->_block_size; }
sia_u32 sia_get_align(si_arena* arena) { return arena->_align; }


void sia_set_global_error_callback(sia_error_callback* callback) {
    _sia_global_error_callback = callback;
}

sia_error_callback* sia_get_global_error_callback(void) {
    return _sia_global_error_callback;
}

void* sia_push_zero(si_arena* arena, sia_u64 size) {
    sia_u8* out = sia_push(arena, size);
    SIA_MEMSET(out, 0, size);
    
    return (void*)out;
}

static sia_b32 _sia_is_valid_ptr(si_arena* arena, void* ptr, sia_u64 size) {
    if (ptr == NULL) return SIA_FALSE;
    
#ifdef SIA_FORCE_MALLOC
    _sia_malloc_node* node = arena->_malloc_backend.cur_node;
    while (node != NULL) {
        sia_u8* node_start = node->data;
        sia_u8* node_end = node_start + node->size;
        sia_u8* ptr_u8 = (sia_u8*)ptr;
        if (ptr_u8 >= node_start && ptr_u8 + size <= node_end) {
            return SIA_TRUE;
        }
        node = node->prev;
    }
    return SIA_FALSE;
#else
    sia_u8* arena_start = (sia_u8*)arena;
    sia_u8* ptr_u8 = (sia_u8*)ptr;
    sia_u8* min_pos = arena_start + SIA_MIN_POS;
    return (ptr_u8 >= min_pos && ptr_u8 + size <= arena_start + arena->_pos);
#endif
}

static sia_b32 _sia_is_last_allocation(si_arena* arena, void* ptr, sia_u64 size) {
#ifdef SIA_FORCE_MALLOC
    _sia_malloc_node* node = arena->_malloc_backend.cur_node;
    sia_u8* node_start = node->data;
    sia_u8* ptr_u8 = (sia_u8*)ptr;
    
    if (ptr_u8 < node_start || ptr_u8 >= node_start + node->size) {
        return SIA_FALSE;
    }
    
    sia_u64 ptr_offset = ptr_u8 - node_start;
    sia_u64 aligned_offset = SIA_ALIGN_UP_POW2(ptr_offset, arena->_align);
    sia_u64 allocation_end = aligned_offset + size;
    
    return (allocation_end == node->pos);
#else
    sia_u8* arena_start = (sia_u8*)arena;
    sia_u8* ptr_u8 = (sia_u8*)ptr;
    
    if (ptr_u8 < arena_start + SIA_MIN_POS || ptr_u8 >= arena_start + arena->_pos) {
        return SIA_FALSE;
    }
    
    sia_u64 ptr_offset = ptr_u8 - arena_start;
    sia_u64 aligned_offset = SIA_ALIGN_UP_POW2(ptr_offset, arena->_align);
    sia_u64 allocation_end = aligned_offset + size;
    
    return (allocation_end == arena->_pos);
#endif
}

si_arena* sia_merge(si_arena** arenas, sia_u32 num_arenas){
    if (arenas == NULL || num_arenas == 0) {
        last_error.code = SIA_ERR_INVALID_PTR;
        last_error.msg = "Arenas are NULL or empty";
        if (_sia_global_error_callback != NULL) {
            _sia_global_error_callback(last_error);
        }
#ifndef SIA_NO_STDIO
        else {
            _sia_stderr_error_callback(last_error);
        }
#endif
        return NULL;
    }

    sia_u64 total_size = 0;
    for (sia_u32 i = 0; i < num_arenas; i++) {
        if (arenas[i] == NULL) {
            last_error.code = SIA_ERR_INVALID_PTR;
            last_error.msg = "Arena is NULL";
            if (_sia_global_error_callback != NULL) {
                _sia_global_error_callback(last_error);
            }
#ifndef SIA_NO_STDIO
            else {
                _sia_stderr_error_callback(last_error);
            }
#endif
            return NULL;
        }
#ifdef SIA_FORCE_MALLOC
        total_size += arenas[i]->_pos;
#else
        total_size += arenas[i]->_pos - SIA_MIN_POS;
#endif
    }

    sia_u32 max_block_size = 0;
    sia_u32 max_align = 0;
    sia_error_callback* error_cb = NULL;
    
    for (sia_u32 i = 0; i < num_arenas; i++) {
        if (arenas[i]->_block_size > max_block_size) {
            max_block_size = arenas[i]->_block_size;
        }
        if (arenas[i]->_align > max_align) {
            max_align = arenas[i]->_align;
        }
        // Use first arena's error callback, or global if available
        if (i == 0) {
            error_cb = arenas[i]->error_callback;
        }
    }
    
    // Use global callback if available, otherwise use first arena's
    if (_sia_global_error_callback != NULL) {
        error_cb = _sia_global_error_callback;
    }

#ifndef SIA_NO_STDIO
    // Default to stderr if no callback set
    if (error_cb == NULL || error_cb == _sia_empty_error_callback) {
        error_cb = _sia_stderr_error_callback;
    }
#endif
    sia_desc merged_desc = {
        .desired_max_size = total_size,
        .desired_block_size = max_block_size,
        .align = max_align, 
        .error_callback = error_cb  
    };

     si_arena* merged = sia_create(&merged_desc);
     if (merged == NULL) {
         return NULL;
     }
     
     for (sia_u32 i = 0; i < num_arenas; i++) {
        sia_u64 merged_pos_before = sia_get_pos(merged);
        si_arena* src = arenas[i];
        sia_u64 src_used = src->_pos;
#ifdef SIA_FORCE_MALLOC
        _sia_malloc_node* node = src->_malloc_backend.cur_node;
        while (node != NULL) {
            sia_u64 copy_size = node->pos;
            if (copy_size > 0){
                void* dst = sia_push(merged, copy_size);
                if (dst == NULL){
                    last_error.code = SIA_ERR_MERGE_FAILED;
                    last_error.msg = "Failed to allocate memory for merge";
                    merged->_last_error = last_error;
                    merged->error_callback(last_error);
                    sia_destroy(merged);
                    return NULL;
                }
                SIA_MEMCPY(dst, node->data, copy_size);
            }
            node = node->prev;
        }
#else
        // Copy from low-level backend: single contiguous copy
        if (src_used > SIA_MIN_POS) {
            sia_u64 copy_size = src_used - SIA_MIN_POS;
            void* src_data = (void*)((sia_u8*)src + SIA_MIN_POS);
            void* dst = sia_push(merged, copy_size);
            if (dst == NULL) {
                last_error.code = SIA_ERR_MERGE_FAILED;
                last_error.msg = "Failed to allocate space in merged arena";
                merged->_last_error = last_error;
                merged->error_callback(last_error);
                sia_destroy(merged);
                return NULL;
            }
            SIA_MEMCPY(dst, src_data, copy_size);
        }
#endif
    }
    
    // Validate merge was successful
    sia_u64 merged_used = sia_get_pos(merged);
#ifdef SIA_FORCE_MALLOC
    if (merged_used != total_size) {
        last_error.code = SIA_ERR_MERGE_FAILED;
        last_error.msg = "Merge validation failed: size mismatch";
        merged->_last_error = last_error;
        merged->error_callback(last_error);
        sia_destroy(merged);
        return NULL;
    }
#else
    if (merged_used - SIA_MIN_POS != total_size) {
        last_error.code = SIA_ERR_MERGE_FAILED;
        last_error.msg = "Merge validation failed: size mismatch";
        merged->_last_error = last_error;
        merged->error_callback(last_error);
        sia_destroy(merged);
        return NULL;
    }
#endif
     
    return merged;
}

void* sia_realloc(si_arena* arena, void* ptr, sia_u64 old_size, sia_u64 new_size) {
    if (arena == NULL) {
        last_error.code = SIA_ERR_INVALID_PTR;
        last_error.msg = "Arena is NULL";
       // Call global callback if available
        if (_sia_global_error_callback != NULL) {
            _sia_global_error_callback(last_error);
        }
        #ifndef SIA_NO_STDIO
        else {
            _sia_stderr_error_callback(last_error);
        }
        #endif
        return NULL;
    }
    
    if (ptr == NULL) {
        // If ptr is NULL, treat as new allocation
        return sia_push(arena, new_size);
    }
    
    if (new_size == 0) {
        // If new_size is 0, treat as free (but we can't actually free in arena)
        last_error.code = SIA_ERR_INVALID_PTR;
        last_error.msg = "New size is 0";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }
    
    if (new_size <= old_size) {
        // Shrinking - in arena, we can't actually shrink, just return same pointer
        // TODO: Validate the pointer is still valid before returning
        if (!_sia_is_valid_ptr(arena, ptr, old_size)) {
            last_error.code = SIA_ERR_INVALID_PTR;
            last_error.msg = "Invalid pointer for realloc";
            arena->_last_error = last_error;
            arena->error_callback(last_error);
            return NULL;
        }
        return ptr;
    }
    
    // Growing allocation
    // TODO: Validate that ptr is a valid pointer from this arena
    if (!_sia_is_valid_ptr(arena, ptr, old_size)) {
        last_error.code = SIA_ERR_INVALID_PTR;
        last_error.msg = "Invalid pointer for realloc";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }
    
    if (_sia_is_last_allocation(arena, ptr, old_size)) {
        // Try to grow in-place
        sia_u64 additional_size = new_size - old_size;
        void* new_ptr = sia_push(arena, additional_size);
        
        if (new_ptr != NULL && new_ptr == (void*)((sia_u8*)ptr + old_size)) {
            return ptr;
        }
        
        // Failed to grow in-place, need to allocate new and copy
        sia_pop(arena, additional_size);
    }
    
#ifdef SIA_FORCE_MALLOC
    // Malloc backend implementation
    // TODO: For malloc backend, check if we can grow within current node
    _sia_malloc_node* node = arena->_malloc_backend.cur_node;

    sia_u8* node_start = node->data;
    sia_u8* node_end = node_start + node->size;
    sia_u8* ptr_u8 = (sia_u8*) ptr;

    // Check if ptr is in current node and there's space after, we can grow in-place
    if (ptr_u8 >= node_start && ptr_u8 < node_end){
        sia_u64 ptr_offset = ptr_u8 - node_start;
        sia_u64 aligned_offset = SIA_ALIGN_UP_POW2(ptr_offset, arena->_align);
        sia_u64 allocation_end = aligned_offset + old_size;

        if (allocation_end == node->pos){
            sia_u64 additional_size = new_size - old_size;
            sia_u64 space_available = node->size - node->pos;
            if (additional_size <= space_available){
                arena->_pos += additional_size;
                node->pos += additional_size;
                return ptr;
            }
        }
    }
    
    // Can't grow in-place, allocate new and copy
    void *new_ptr = sia_push(arena, new_size);
    if (new_ptr == NULL){
        last_error.code = SIA_ERR_REALLOC_FAILED;
        last_error.msg = "Failed to allocate new memory for realloc";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }
    SIA_MEMCPY(new_ptr, ptr, old_size);
    return new_ptr;
#else
    // Low-level backend implementation
    sia_u8* arena_start = (sia_u8*)arena;
    sia_u8* ptr_u8 = (sia_u8*)ptr;

    // Check if ptr is within valid arena range
    if (ptr_u8 >= arena_start + SIA_MIN_POS && ptr_u8 < arena_start + arena->_pos) {
        sia_u64 ptr_offset = ptr_u8 - arena_start;
        sia_u64 aligned_offset = SIA_ALIGN_UP_POW2(ptr_offset, arena->_align);
        sia_u64 allocation_end = aligned_offset + old_size;

        if (allocation_end == arena->_pos){
            sia_u64 additional_size = new_size - old_size;
            sia_u64 space_available = arena->_size - arena->_pos;
            if (additional_size <= space_available){
                arena->_pos += additional_size;
                sia_u64 commit_pos = arena->_reserve_backend.commit_pos;
                if (arena->_pos > commit_pos){
                    sia_u64 commit_unclamped = SIA_ALIGN_UP_POW2(arena->_pos, arena->_block_size);
                    sia_u64 new_commit_pos = SIA_MIN(commit_unclamped, arena->_size);
                    sia_u64 commit_size = new_commit_pos - commit_pos;
                    if (!SIA_MEM_COMMIT((void*)(arena_start + commit_pos), commit_size)){
                        // Rollback arena->_pos on commit failure
                        arena->_pos -= additional_size;
                        last_error.code = SIA_ERR_COMMIT_FAILED;
                        last_error.msg = "Failed to commit memory for realloc";
                        arena->_last_error = last_error;
                        arena->error_callback(last_error);
                        return NULL;
                    }
                    arena->_reserve_backend.commit_pos = new_commit_pos;

                }
                return ptr;
            }
        }
    }
    
    // Can't grow in-place, allocate new and copy
    void* new_ptr = sia_push(arena, new_size);
    if (new_ptr == NULL) {
        last_error.code = SIA_ERR_REALLOC_FAILED;
        last_error.msg = "Failed to allocate new memory for realloc";
        arena->_last_error = last_error;
        arena->error_callback(last_error);
        return NULL;
    }
    
    // Copy old_size bytes from ptr to new_ptr
    SIA_MEMCPY(new_ptr, ptr, old_size);
    
    return new_ptr;
#endif
}

void sia_pop_to(si_arena* arena, sia_u64 pos) {
    sia_pop(arena, arena->_pos - pos);
}

sia_temp sia_temp_begin(si_arena* arena) {
    return (sia_temp){
        .arena = arena,
        ._pos = arena->_pos
    };
}
void sia_temp_end(sia_temp temp) {
    sia_pop_to(temp.arena, temp._pos);
}

#ifndef SIA_SCRATCH_COUNT
#   define SIA_SCRATCH_COUNT 2
#endif

#ifndef SIA_NO_STDIO
static void _sia_scratch_on_error(sia_error err) {
    fprintf(stderr, "SIA Scratch Error %u: %s\n", err.code, err.msg);
}
#endif

static SIA_THREAD_VAR sia_desc _sia_scratch_desc = {
    .desired_max_size = SIA_MiB(64),
    .desired_block_size = SIA_KiB(256),
#ifndef SIA_NO_STDIO
    .error_callback = _sia_scratch_on_error,
#endif
};
static SIA_THREAD_VAR si_arena* _sia_scratch_arenas[SIA_SCRATCH_COUNT] = { 0 };

void sia_scratch_set_desc(const sia_desc* desc) {
    if (_sia_scratch_arenas[0] == NULL) {
        _sia_scratch_desc = (sia_desc){
            .desired_max_size = desc->desired_max_size,
            .desired_block_size = desc->desired_block_size,
            .align = desc->align,
            .error_callback = desc->error_callback
        };
    }
}
sia_temp sia_scratch_get(si_arena** conflicts, sia_u32 num_conflicts) {
    if (_sia_scratch_arenas[0] == NULL) {
        for (sia_u32 i = 0; i < SIA_SCRATCH_COUNT; i++) {
            _sia_scratch_arenas[i] = sia_create(&_sia_scratch_desc);
        }
    }

    sia_temp out = { 0 };

    for (sia_u32 i = 0; i < SIA_SCRATCH_COUNT; i++) {
        si_arena* arena = _sia_scratch_arenas[i];

        sia_b32 in_conflict = SIA_FALSE;
        for (sia_u32 j = 0; j < num_conflicts; j++) {
            if (arena == conflicts[j]) {
                in_conflict = SIA_TRUE;
                break;
            }
        }
        if (in_conflict) { continue; }

        out = sia_temp_begin(arena);
    }

    return out;
}
void sia_scratch_release(sia_temp scratch) {
    sia_temp_end(scratch);
}

#ifdef __cplusplus
}
#endif

#endif // SI_ARENA_IMPL

/*
License
=================================
  _    ___ ___ ___ _  _ ___ ___ 
 | |  |_ _/ __| __| \| / __| __|
 | |__ | | (__| _|| .` \__ \ _| 
 |____|___\___|___|_|\_|___/___|
                                
=================================

MIT License

Copyright (c) 2023 Magicalbat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
