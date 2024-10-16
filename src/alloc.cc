#include "alloc.hh"
#include "core.hh"

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

char* alligned_alloc_zeroed_cpu(size_t sz)
{
	void* ptr = malloc(sz + 16);

	if (!ptr) {
		printf("failed to alloc %zu B\n", sz);
		exit(1);
	}

	memset(ptr, 0, sz + 16);

	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned = (addr + 16) & 0xfffffffffffffff0;

	return (char*)aligned;
}

char* aligned_alloc_gpu(size_t sz)
{
	void* ptr = gpu_malloc(sz + 16);
	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned = (addr + 16) & 0xfffffffffffffff0;

	return (char*)aligned;
}

void* malloc_check(size_t sz)
{
	void* ptr = malloc(sz);

	if (!ptr) {
		printf("failed to malloc %zu B\n", sz);
		exit(1);
	}

	return ptr;
}
