#pragma once

#include "core.hh"

struct Map4
{
	int sx;
	int sy;
	int sz;
	int slabs_size;
	uint* map;
	ushort* slabs;
};

struct RLE4
{
	Map4 map[16];
	Map4 mapgpu[16];
	int nummaps;

	void init();
	void clear();
	void save(char* filename);
	bool load(char* filename);
	Map4 copy_to_gpu(Map4 map4);
	void all_to_gpu();
	void all_to_gpu_tex();
	void setcol(long x, long y, long z, long argb);
	long loadvxl(char* filnam);
	Map4 compressvxl(ushort* mem, int sx, int sy, int sz, int mip_lvl);
};
