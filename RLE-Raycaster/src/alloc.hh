#pragma once

#include <cstddef>

struct arena {
	char *beg;
	char *end;
};

void  arena_init(void* pool, size_t sz);
void* arena_alloc(arena* a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count);
void* bmalloc(ptrdiff_t size);
