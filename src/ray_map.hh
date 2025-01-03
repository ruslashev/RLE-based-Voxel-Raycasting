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
	int total_rays;
	vec3f rotation; // Frustum orientation
	vec3f position; // Frustum position
	float border;
	Map4 map4_gpu[16];
	int nummaps;
	int maxres;
	int res[4];
	vec3f vp;
	vec3f p_no[8];
	matrix44 to3d;
	float p_ofs_min[4];

	RayMap();
	void set_border(float a);
	void get_ray_map(vec3f pos, vec3f rot);
};
