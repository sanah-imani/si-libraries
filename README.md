# MG Libraries

## STB-Style single header libraries

| Library | Docs | Description |
| ------- | ---- | ----------- |
| [mg_arena.h](mg_arena.h) | [MG Arena](docs/mg_arena.md) | Arena Memory Managment 


## General Installation
Generally, to use one of these libraries, you should make a separate file (something like `mg_impl.c`), and put the following in the file:
```c
#define MG_*_IMPL
#include "mg_*.h"
```
Some libraries may require more steps and/or custom compile instructions.


## [MG Arena](mg_arena.h) ([Docs](docs/mg_arena.md))
A small library for memory arenas in C.

Example:
```c
#include <stdio.h>

// You should put the implementation in a separate source file in a real project
#define MG_ARENA_IMPL
#include "mg_arena.h"

void arena_error(mga_error err) {
    fprintf(stderr, "MGA Error %d: %s\n", err.code, err.msg);
}

int main() {
    mg_arena* arena = mga_create(&(mga_desc){
        .desired_max_size = MGA_MiB(4),
        .desired_block_size = MGA_KiB(256),
        .error_callback = arena_error
    });

    int* data = (int*)mga_push(arena, sizeof(int) * 64);
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    printf("[ ");
    for (int i = 0; i < 64; i++) {
        printf("%d, ", data[i]);
    }
    printf("\b\b ]\n");

    mga_destroy(arena);

    return 0;
}
```