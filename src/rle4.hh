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
	bool load(const char* filename);
	Map4 copy_to_gpu(Map4 map4);
	void all_to_gpu();
};
