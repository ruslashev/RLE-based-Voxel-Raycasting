#pragma once

#include <cstddef>

struct arena {
	char *beg;
	char *end;
};

void  arena_init(void* pool, size_t sz);
void* arena_alloc(arena* a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count);
void* bmalloc(ptrdiff_t size);
char* alligned_alloc_zeroed_cpu(size_t sz);
char* aligned_alloc_gpu(size_t sz);
void* malloc_check(size_t sz);
