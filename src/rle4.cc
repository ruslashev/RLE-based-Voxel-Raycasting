#include "rle4.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef IN_CUDA_ENV
  typedef float3 vec3f;
  typedef int3 vec3i;
#else
  #include "alloc.hh"
#endif

void RLE4::init()
{
	nummaps = 0;
}

void RLE4::clear()
{
	for (int m = 0; m < nummaps; m++) {
		if (map[m].map)   free(map[m].map);
		if (map[m].slabs) free(map[m].slabs);
	}
	init();
}

static void* malloc_check(size_t sz)
{
	void* ptr = malloc(sz);

	if (!ptr) {
		printf("failed to malloc %zu B\n", sz);
		exit(1);
	}

	return ptr;
}

bool RLE4::load(const char* filename)
{
	FILE* fn;

	if ((fn = fopen(filename, "rb")) == NULL) {
		printf("File '%s' not found\n", filename);
		return false;
	}

	int filesize = 0;

	printf("Loading %s\n", filename);

	fread(&nummaps, 1, 4, fn);
	filesize += 4;

	for (int m = 0; m < nummaps; m++) {
		fread(&map[m].sx, 1, 4, fn);
		fread(&map[m].sy, 1, 4, fn);
		fread(&map[m].sz, 1, 4, fn);
		fread(&map[m].slabs_size, 1, 4, fn);
		filesize += 16;

		map[m].map = (uint*)malloc_check(map[m].sx * map[m].sz * 4 * 2);
		map[m].slabs = (ushort*)malloc_check(map[m].slabs_size * 2);

		fread(map[m].slabs, 1, map[m].slabs_size * 2, fn);
		filesize += map[m].slabs_size * 2;

		int x = 0, z = 0;

		memset(map[m].map, 0, map[m].sx * map[m].sz * 4 * 2);
		map[m].map[0] = 0;

		int ofs = 0;

		int numrle = 0, numtex = 0;

		while (1) {
			map[m].map[(x + z * map[m].sx) * 2 + 0] = ofs;

			uint count = map[m].slabs[ofs];
			uint firstrle = 0;
			if (ofs + 2 < map[m].slabs_size)
				firstrle = map[m].slabs[ofs + 2];

			map[m].map[(x + z * map[m].sx) * 2 + 1] = count + (firstrle << 16);

			x = (x + 1) % map[m].sx;
			if (x == 0) {
				z = (z + 1) % map[m].sz;
				if (z == 0)
					break;
			}
			numrle += map[m].slabs[ofs] + 2;
			numtex += map[m].slabs[ofs + 1];
			ofs += map[m].slabs[ofs] + map[m].slabs[ofs + 1] + 2;
		}
	}

	fclose(fn);
	return true;
}

void RLE4::all_to_gpu()
{
	for (int m = 0; m < nummaps; m++)
		mapgpu[m] = copy_to_gpu(map[m]);
}

Map4 RLE4::copy_to_gpu(Map4 map4)
{
	Map4 map4gpu = map4;

	size_t map_len = map4.sx * map4.sz * 4 * 2;
	size_t slabs_len = map4.slabs_size * 2;

	uintptr_t map_gpu   = (uintptr_t)bmalloc(map_len + 32)   + cpu_to_gpu_delta;
	uintptr_t slabs_gpu = (uintptr_t)bmalloc(slabs_len + 32) + cpu_to_gpu_delta;

	map4gpu.map   = (uint*)((map_gpu + 31)     & 0xffffffffffffffe0);
	map4gpu.slabs = (ushort*)((slabs_gpu + 31) & 0xffffffffffffffe0);

	gpu_memcpy((char*)map4gpu.map, (char*)map4.map, map_len);
	gpu_memcpy((char*)map4gpu.slabs, (char*)map4.slabs, slabs_len);

	return map4gpu;
}
