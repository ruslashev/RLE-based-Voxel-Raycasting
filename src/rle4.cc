#include "rle4.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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

void RLE4::save(char* filename)
{
	FILE* fn;

	if ((fn = fopen(filename, "wb")) == NULL)
		return;
	printf("Saving %s\n", filename);

	fwrite(&nummaps, 1, 4, fn);

	if (nummaps == 0) {
		puts("0 mipmaps");
		exit(0);
	}

	for (int m = 0; m < nummaps; m++) {
		printf("MipVol %d ------------------------\n", m);
		fwrite(&map[m].sx, 1, 4, fn);
		fwrite(&map[m].sy, 1, 4, fn);
		fwrite(&map[m].sz, 1, 4, fn);
		fwrite(&map[m].slabs_size, 1, 4, fn);
		fwrite(map[m].slabs, 1, map[m].slabs_size * 2, fn);
	}

	fclose(fn);
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

bool RLE4::load(char* filename)
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

void RLE4::all_to_gpu_tex()
{
	int mapofs_y = 0;
	int mapadd_y = 1024;
	int slabs_total = 0;

	for (int m = 0; m < nummaps; m++)
		slabs_total += map[m].slabs_size;

	char* slabs_mem = (char*)malloc_check(slabs_total * 2);

	slabs_total = 0;
	for (int m = 0; m < nummaps; m++) {
		for (int a = 0; a < map[m].sx * map[m].sx; a++)
			map[m].map[a * 2] += slabs_total;
		memcpy(slabs_mem + slabs_total * 2, map[m].slabs, map[m].slabs_size * 2);
		slabs_total += map[m].slabs_size;
	}

	create_cuda_1d_texture((char*)slabs_mem, slabs_total * 2);

	char* mapdata64 = (char*)malloc_check(map[0].sx * 8 * map[0].sz * 2);

	for (int m = 0; m < nummaps; m++) {
		for (int y = 0; y < map[m].sy; y++)
			memcpy(mapdata64 + map[0].sx * 8 * (mapofs_y + y),
					((char*)map[m].map) + y * map[m].sx * 8, map[m].sx * 8);

		mapofs_y += mapadd_y;
		mapadd_y >>= 1;
	}

	create_cuda_2d_texture((uint*)mapdata64, map[0].sx, map[0].sz * 2);
	free(mapdata64);
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

// Called only for surface voxels
// A surface voxel is any solid voxel with at least 1 air voxel
//   on one of its 6 sides. All solid voxels at z=0 are automatically
//   surface voxels, but this is not true for x=0, x=1023, y=0, y=1023,
//   z=255 (I believe)
// argb: 32-bit color, high byte is used for shading scale (can be ignored)
static ushort* volu;
void RLE4::setcol(long x, long y, long z, long argb)
{
	int ofs = x + z * 1024 + y * 256 * 1024;

	uint rgb = argb;
	uint r = ((rgb >> 0) & 255) >> 3;
	uint g = ((rgb >> 8) & 255) >> 3;
	uint b = ((rgb >> 16) & 255) >> 3;
	volu[ofs] = r + (g << 5) + (b << 10) + (1 << 15);
}

long RLE4::loadvxl(char* filnam)
{
	struct dpoint3d {
		double x, y, z;
	};
	dpoint3d ipos, istr, ihei, ifor;

	FILE* fil;
	long i, x, y, z;
	unsigned char *v, *vbuf;

	fil = fopen(filnam, "rb");
	if (!fil)
		return (-1);
	fread(&i, 4, 1, fil);
	if (i != 0x09072000)
		return (-1);
	fread(&i, 4, 1, fil);
	if (i != 1024)
		return (-1);
	fread(&i, 4, 1, fil);
	if (i != 1024)
		return (-1);
	fread(&ipos, 24, 1, fil); // camera position
	fread(&istr, 24, 1, fil); // unit right vector
	fread(&ihei, 24, 1, fil); // unit down vector
	fread(&ifor, 24, 1, fil); // unit forward vector

	printf("loading %s...\n", filnam);

	// Allocate huge buffer and load rest of file into it...

	int p1 = ftell(fil);
	fseek(fil, 0L, SEEK_END);
	int p2 = ftell(fil);
	fseek(fil, p1, SEEK_SET);

	printf("reading %d bytes...\n", p2 - p1);

	i = p2 - p1;
	vbuf = (unsigned char*)malloc_check(i);
	fread(vbuf, i, 1, fil);
	fclose(fil);

	printf("clear volume\n");

	/*
	 * 0 numskip
	 * 1 start
	 * 2 end1
	 * 3 end2
	 * 4 rgba
	 * 8 rgba
	 */

	int scale = 0; // 1<<mip_lvl;
	int sx = 1024;
	int sy = 256;
	int sz = 1024;

	ushort* volume = (ushort*)malloc_check(sx * sy * sz * 2);

	memset(volume, 0, sx * sy * sz * 2);

	volu = volume;

	Map4 map4;

	map4.sx = sx;
	map4.sy = sy;
	map4.sz = sz;
	int sxy = sx * sy;

	printf("unpack volume\n");

	v = vbuf;
	for (y = 0; y < 1024; y++)
		for (x = 0; x < 1024; x++) {
			while (1) {
				for (z = v[1]; z <= v[2]; z++)
					setcol(x, y, z, *(long*)&v[(z - v[1] + 1) << 2]);

				if (!v[0])
					break;

				z = v[2] - v[1] - v[0] + 2;
				v += v[0] * 4;

				for (z += v[3]; z < v[3]; z++)
					setcol(x, y, z, *(long*)&v[(z - v[3]) << 2]);
			}
			v += ((((long)v[2]) - ((long)v[1]) + 2) << 2);
		}

	free(vbuf);

	printf("compressing\n");
	Map4 m = compressvxl(volume, 1024, 256, 1024, 0);

	map[0] = m;
	nummaps = 1;

	return 0;
}

Map4 RLE4::compressvxl(ushort* mem, int sx, int sy, int sz, int mip_lvl)
{
	int scale = 1 << mip_lvl;

	Map4 map4;

	map4.sx = sx;
	map4.sy = sy;
	map4.sz = sz;
	int sxy = sx * sy;

	std::vector<ushort> slab;
	std::vector<ushort> texture;
	std::vector<uint> map;

	map.resize(sx * sz);

	bool store = false;

	for (int i = 0; i < sx; i++)
		for (int k = 0; k < sz; k++) {
			map[i + k * sx] = slab.size();

			int slab_len = slab.size();
			slab.push_back(0);
			int tex_len = slab.size();
			slab.push_back(0);

			int skip = 0, solid = 0;

			texture.clear();

			for (int j = 0; j < sy + 1; j++) {
				ushort f = 0;

				if (j < sy)
					f = mem[(i + j * sx + k * sxy)] & (1 << (15));

				store = false;

				int nx, ny, nz;

				if (f) {
					int cnt = 0;
					int mx = 0;
					int my = 0;
					int mz = 0;

					for (int a = -1; a < 2; a++)
						for (int b = -1; b < 2; b++)
							for (int c = -1; c < 2; c++) {
								int x = i + a;
								int y = j + b;
								int z = k + c;

								if (x < 0) continue;
								if (y < 0) continue;
								if (z < 0) continue;
								if (x >= sx) continue;
								if (y >= sy) continue;
								if (z >= sz) continue;

								ushort d = mem[(x + y * sx + z * sxy)] & (1 << 15);

								if (d) {
									mx += a;
									my += b;
									mz += c;
									cnt++;
								}
							}
					if (cnt < 27) {
						texture.push_back(mem[(i + j * sx + k * sxy)] & (32767));
						store = true;
					}
				}

				if (solid > 0)
					if (!store) {
						while (skip > 1023) {
							slab.push_back(1023);
							skip -= 1023;
						}
						while (solid > 63) {
							slab.push_back(63 * 1024 + (skip & 1023));
							solid -= 63;
							skip = 0;
						}
						slab.push_back((solid & 63) * 1024 + (skip & 1023));
						solid = 0;
						skip = 0;
					}

				if (store)
					solid++;
				else
					skip++;
			}
			slab[slab_len] = slab.size() - (slab_len + 2);
			slab[tex_len] = texture.size();
			for (int i = 0; i < texture.size(); i++)
				slab.push_back(texture[i]);
		}

	map4.map = (uint*)malloc_check(sx * sz * 4);
	memcpy(map4.map, &map[0], sx * sz * 4);

	map4.slabs = (ushort*)malloc_check(slab.size() * 2);
	memcpy(map4.slabs, &slab[0], slab.size() * 2);

	map4.slabs_size = slab.size();
	return map4;
}
