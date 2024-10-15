#include "alloc.hh"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static arena a;

void arena_init(void* pool, size_t sz)
{
	a.beg = (char*)pool;
	a.end = (char*)pool + sz;
}

void* arena_alloc(arena* a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count)
{
	ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
	ptrdiff_t available = a->end - a->beg - padding;

	if (available < 0 || count > available / size) {
		printf("alloc: failed to alloc %zu B (%zu MB)\n", size, size >> 20);
		exit(1);
	}

	void *p = a->beg + padding;

	a->beg += padding + count * size;

	return memset(p, 0, count * size);
}

void* bmalloc(ptrdiff_t size)
{
	return arena_alloc(&a, size, 16, 1);
}
