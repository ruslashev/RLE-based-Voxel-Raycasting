#pragma once

#include "rle4.hh"

#ifdef IN_CUDA_ENV
  typedef float3 vec3f;
  typedef int3 vec3i;
  struct matrix44 { float m[4][4]; };
#else
  #include "../inc/mathlib/matrix.h"
#endif

struct RayMap
{
	vec3f vanishing_point_2d;
	int map_line_count;
	int map_line_limit;
	vec3f rotation; // Frustum orientation
	vec3f position; // Frustum position
	float border;
	float clip_min, clip_max;
	Map4 map4_gpu[16];
	int nummaps;
	int maxres;
	int res[4];
	vec3f p4;
	vec3f p_2d[8];
	vec3f p_no[8];
	matrix44 to3d;
	float p_ofs_min[4];
	float p_ofs_max[4];

	RayMap();
	void set_border(float a);
	void set_ray_limit(int a);
	void get_ray_map(vec3f pos, vec3f rot);
};
