#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define IN_CUDA_ENV

#include "../inc/cutil_math.h"
#include "../inc/cutil.h"
#include "../inc/mathlib/matrixdefs.h"
#include <cuda_gl_interop.h>

#include "alloc.hh"
#include "ray_map.hh"
#include "rle4.hh"

texture<uint2, 2, cudaReadModeElementType> texture_pointermap;
texture<unsigned short, 1, cudaReadModeElementType> texture_slabs;

cudaArray* cu_array;
cudaChannelFormatDesc channelDesc;

cudaArray* cu_array_pointermap;
cudaChannelFormatDesc channelDesc_pointermap;

struct Render
{
	RayMap ray_map;
	int res_x, res_y;
	int* data_rgb;

	void set_target(int resolution_x, int resolution_y, int* data_rgb)
	{
		this->res_x = resolution_x;
		this->res_y = resolution_y;
		this->data_rgb = data_rgb;
		if (data_rgb == 0) {
			printf("data_rgb == 0\n");
			while (1) {}
		}
	}

	void set_raymap(RayMap* raymap)
	{
		memcpy(&ray_map, raymap, sizeof(RayMap));
	}

	inline __device__ float LineScale(vec3f input, vec3f center, float clip_max, float clip_min)
	{
		float scale_x = 1;
		float scale_y = 1;

		if (center.x > 1) scale_x = (1 - input.x) / (center.x - input.x);
		if (center.x < 0) scale_x = input.x / (input.x - center.x);
		if (center.y > clip_max) scale_y = (clip_max - input.y) / (center.y - input.y);
		if (center.y < clip_min) scale_y = (-clip_min + input.y) / (input.y - center.y);

		float scale = (scale_x < scale_y) ? scale_x : scale_y;

		return scale;
	}

	inline __device__ void ClipLine(vec3f& p1, vec3f& p2, float clip_max, float clip_min)
	{
		vec3f c1 = p1;
		vec3f c2 = p2;
		float scale;

		scale = LineScale(p1, p2, clip_max, clip_min);
		c2 = p1 + (p2 - p1) * scale;
		scale = LineScale(p2, p1, clip_max, clip_min);
		c1 = p2 + (p1 - p2) * scale;

		p1 = c1;
		p2 = c2;
	}

	inline __device__ vec3f MatMul(matrix44 m, vec3f v)
	{
		return make_float3(m.M11 * v.x + m.M21 * v.y + m.M31 * v.z + m.M41,
				m.M12 * v.x + m.M22 * v.y + m.M32 * v.z + m.M42,
				m.M13 * v.x + m.M23 * v.y + m.M33 * v.z + m.M43);
	}

	inline __device__ void vec3f_rot_y(float a, vec3f& v)
	{
		float xx = cos(-a) * v.x + sin(-a) * v.z;
		float zz = cos(-a) * v.z - sin(-a) * v.x;
		v.x = xx;
		v.z = zz;
	}

	inline __device__ void render_line(
			int x,
			unsigned int* y_cache,
			vec3f viewpos,
			vec3f viewrot,
			int res_x,
			int res_y,
			ushort* ofs_skip_start)
	{
		float res_x2 = res_x / 2;
		float res_y2 = res_y / 2;
		uint* ofs_rgb_start = (uint*)&data_rgb[x * res_y];
		uint* ofs_cache_start = ((uint*)ofs_skip_start) + x * res_y;

		ofs_cache_start[0] = 0;
		uint ofs_cache_count = 0;
		uint ofs_cache_depth = 0;

		float ml_ray_x;
		float ml_ray_z;
		vec3f ml_start2d;
		vec3f ml_end2d;
		vec3f ml_start3d;
		vec3f ml_end3d;
		bool ml_direction_y;

		{
			int rays[4];
			rays[0] = ray_map.res[0];
			rays[1] = ray_map.res[1] + rays[0];
			rays[2] = ray_map.res[2] + rays[1];
			rays[3] = ray_map.res[3] + rays[2];

			int quadrant = 0;
			if (x >= rays[2])
				quadrant = 3;
			else if (x >= rays[1])
				quadrant = 2;
			else if (x >= rays[0])
				quadrant = 1;

			float quadrant_ofs = x;
			if (quadrant >= 1)
				quadrant_ofs -= rays[quadrant - 1];

			float quadrant_num = ray_map.res[quadrant];
			float a = quadrant_ofs / quadrant_num;

			int j = quadrant;

			vec3f p1, p2, p1_3d, p2_3d;
			p1 = ray_map.vp;
			p2 = ray_map.p_no[j * 2] + (ray_map.p_no[j * 2 + 1] - ray_map.p_no[j * 2]) * a;

			ClipLine(p1, p2, ray_map.clip_max, ray_map.clip_min);

			matrix44 to3d = ray_map.to3d;
			vec3f p1m4 = p1 * 4.0;
			vec3f p2m4 = p2 * 4.0;
			p1_3d = MatMul(to3d, p1m4);
			p2_3d = MatMul(to3d, p2m4);

			vec3f delta = (p1_3d + p2_3d) * 0.5; // - origin
			delta.y = 0;
			delta = normalize(delta);
			vec3f_rot_y(viewrot.y, delta);

			ml_ray_x = delta.x;
			ml_ray_z = delta.z;
			ml_start2d = p1;
			ml_end2d = p2;
			ml_start3d = p1_3d;
			ml_end3d = p2_3d;
			ml_direction_y = 1 - (j >> 1);
		}

		// Initialize Render Vars
		int mip_lvl = 0;
		int y_clip_min = 0;
		int y_clip_max = res_y - 1;
		const int z_far = RAYS_DISTANCE;
		float dz = 1 << mip_lvl;
		int mapswitch = MIP_DISTANCE; // res_y2;

		// if (SCREEN_SIZE_X < res_y) mapswitch	= SCREEN_SIZE_X;

		// Initialize Rotation Vars
		float sin_x = sin(ray_map.rotation.x); // Rotation around x-axis
		float cos_x = cos(ray_map.rotation.x);
		float sin_y = sin(ray_map.rotation.y); // Rotation around y-axis
		float cos_y = cos(ray_map.rotation.y);

		// Initialize Raymap Vars
		float ray_x = ml_ray_x;
		float ray_z = ml_ray_z;
		bool vertical = ml_direction_y;

		// Reverse texturing ?
		bool reverse = false;
		if (vertical) if (ray_z <= 0) reverse = true;
		if (!vertical) if (ray_x <= 0) if (sin_x > 0) reverse = true;
		if (!vertical) if (ray_x > 0) if (sin_x < 0) reverse = true;

		float res_x2_mul_reverse = reverse ? -res_x2 : res_x2;

		if (vertical)
			res_x2_mul_reverse = -res_x2_mul_reverse;

		// Screenspace clipping
		int3 p1, p2;
		int p_add = reverse ? 1 : -2;
		p1.x = (int)((float)res_x * ml_start2d.x) + p_add;
		p1.y = (int)((float)res_y * ml_start2d.y) + p_add;
		p2.x = (int)((float)res_x * ml_end2d.x) - p_add;
		p2.y = (int)((float)res_y * ml_end2d.y) - p_add;
		if (p1.x < 0) p1.x = 0; if (p1.x >= res_x) p1.x = res_x - 1;
		if (p1.y < 0) p1.y = 0; if (p1.y >= res_y) p1.y = res_y - 1;
		if (p2.x < 0) p2.x = 0; if (p2.x >= res_x) p2.x = res_x - 1;
		if (p2.y < 0) p2.y = 0; if (p2.y >= res_y) p2.y = res_y - 1;

		if (p1.y == p2.y)
			return; // If removed -> Error ..!!?? Todo

		y_clip_min = res_x - 1 - p1.x;
		y_clip_max = res_x - 1 - p2.x;

		if (vertical) {
			y_clip_min = res_y - 1 - p1.y;
			y_clip_max = res_y - 1 - p2.y;
		}

		if (reverse) {
			y_clip_min = res_y - 1 - y_clip_min;
			y_clip_max = res_y - 1 - y_clip_max;
		}

		if (y_clip_min > y_clip_max) {
			int tmp = y_clip_min;
			y_clip_min = y_clip_max;
			y_clip_max = tmp;
		}

		if (y_clip_min >= y_clip_max)
			return;

		// Clear current rendered Line
		for (int n = y_clip_min; n <= y_clip_max; n++) {
			ofs_rgb_start[n] = 0xff8844;
#ifdef PERPIXELFORWARD
			ofs_skip_start[n] = 0;
#endif
		}

#ifdef SHAREMEMCLIP
		for (int n = 0; n < 31; n++)
			y_cache[n] = 0;
#endif

		float2 direction_rot;
		direction_rot.x = ray_x * cos_y + ray_z * sin_y;
		direction_rot.y = ray_x * sin_y - ray_z * cos_y;

		float2 delta = direction_rot, frac, fix, sign;

		fix.x = fix.y = -1;
		frac.x = viewpos.x - int(viewpos.x);
		frac.y = viewpos.z - int(viewpos.z);
		sign.x = sign.y = -1;

		// Signs & direction for frac
		if (delta.x >= 0) {
			fix.x = 0;
			sign.x = 1;
			frac.x = 1 - frac.x;
		}
		if (delta.y >= 0) {
			fix.y = 0;
			sign.y = 1;
			frac.y = 1 - frac.y;
		}

		// Gradients
		float2 grad0, grad1;
		grad0.y = delta.y / fabs(delta.x);
		grad0.x = sign.x;
		grad1.x = delta.x / fabs(delta.y);
		grad1.y = sign.y;

		// Intersections in x-,y- and z-plane
		float2 isect0, isect1;
		isect0.x = grad0.x * frac.x;
		isect0.y = grad0.y * frac.x;
		isect1.x = grad1.x * frac.y;
		isect1.y = grad1.y * frac.y;

		float grad_dist0 = sqrt(grad0.x * grad0.x + grad0.y * grad0.y);
		float grad_dist1 = sqrt(grad1.x * grad1.x + grad1.y * grad1.y);
		float dds_dist0 = sqrt(isect0.x * isect0.x + isect0.y * isect0.y);
		float dds_dist1 = sqrt(isect1.x * isect1.x + isect1.y * isect1.y);

		float2 pos_vxl_before = { 0, 0 };
		float dds_dist_before = 0;
		float2 pos_vxl = { 0, 0 };
		float dds_dist_now = 0;

		int index = 0, index_before = 0;

		// Main Render Loop
		int rle4_gridx = ray_map.map4_gpu[mip_lvl].sx;
		int rle4_gridz = ray_map.map4_gpu[mip_lvl].sz;

		float pos3d_z_add = sin_x;
		float pos3d_y_add = vertical ? cos_x : 0;
		pos3d_y_add *= res_x2_mul_reverse;

		uint* map_ptr = ray_map.map4_gpu[mip_lvl].map;
		ushort* slab_ptr = ray_map.map4_gpu[mip_lvl].slabs;

		float z = 0;

		float y_map_switch = viewpos.y;

		mapswitch = mapswitch * (0.25 * (4 - abs(viewrot.x)));

		uint tex_map_ofs = 0;

		while (true) {
			ofs_cache_depth++;

			while (z > mapswitch || y_map_switch > 512.0) {
				y_map_switch = y_map_switch * 0.5;

				if (mip_lvl < ray_map.nummaps - 1) {
					mip_lvl++;
					tex_map_ofs += rle4_gridz;
					rle4_gridx >>= 1;
					rle4_gridz >>= 1;
					map_ptr = ray_map.map4_gpu[mip_lvl].map;
					slab_ptr = ray_map.map4_gpu[mip_lvl].slabs;
				}

				grad0.x *= 2;
				grad0.y *= 2;
				grad1.x *= 2;
				grad1.y *= 2;
				grad_dist0 *= 2;
				grad_dist1 *= 2;
				mapswitch *= 2;
				dz *= 2;
			}

			z += dz;
			if (z > z_far)
				return;

			if ((y_clip_min >= y_clip_max))
				return;

			// DDA
			dds_dist_before = dds_dist_now;
			pos_vxl_before = pos_vxl;
			index_before = index;

			if (dds_dist1 < dds_dist0) {
				dds_dist_now = dds_dist1;
				index = 1;
				dds_dist1 += grad_dist1;
				pos_vxl = isect1;
				isect1.x += grad1.x;
				isect1.y += grad1.y;
			} else {
				dds_dist_now = dds_dist0;
				index = 0;
				pos_vxl = isect0;
				dds_dist0 += grad_dist0;
				isect0.x += grad0.x;
				isect0.y += grad0.y;
			}

			int fix_x = (1 - index_before) * fix.x, fix_z = (index_before)*fix.y;

			float dds_dist_delta = dds_dist_now - dds_dist_before;

			vec3f view_space;
			view_space.x = ray_x * dds_dist_before;
			view_space.z = ray_z * dds_dist_before;

			int voxel_x = (int(viewpos.x + pos_vxl_before.x) + fix_x);
			int voxel_z = (int(viewpos.z + pos_vxl_before.y) + fix_z);

#ifdef CLIPREGION
			if (voxel_x < 0) continue;
			if (voxel_z < 0) continue;
			if ((voxel_x >> mip_lvl) > rle4_gridx - 1) continue;
			if ((voxel_z >> mip_lvl) > rle4_gridz - 1) continue;
#endif

			int vx = (voxel_x >> mip_lvl) & (rle4_gridx - 1);
			int vz = (voxel_z >> mip_lvl) & (rle4_gridz - 1);

			float correct_x = ray_x * dds_dist_delta;
			float correct_z = ray_z * dds_dist_delta;

			float pos3d_z = cos_x * (view_space.z) + sin_x * viewpos.y;
			float pos3d_y = (vertical) ? cos_x * viewpos.y - sin_x * (view_space.z) : view_space.x;

			pos3d_y *= res_x2_mul_reverse;

			// Clip Top //if (pos3d_z>0)
			if (pos3d_z * res_y2 + pos3d_y <= pos3d_z * y_clip_min)
				continue;

			uint2 int64_ofs_rle = ((uint2*)map_ptr)[vx + vz * rle4_gridx];

			uint slab_offset = int64_ofs_rle.x;
			uint len_first = int64_ofs_rle.y;
			ushort slen = len_first;

			float pos3d_z1, pos3d_z2;
			int scr_y1;
			float pos3d_y1, pos3d_y2;
			int scr_y2;

			float corr_zz = cos_x * correct_z;
			float corr_yy = (vertical) ? -sin_x * correct_z : correct_x;

			corr_yy *= res_x2_mul_reverse;

			int sti_general = 0;
			int sti_skip = 0;
			ushort sti_;

			sti_ = len_first >> 16;

			ushort* slabs = slab_ptr + 2 + slab_offset;
			ushort* send = slabs + slen;

			float tex = 0;
			sti_general = 0;
			sti_skip = 0;

			uintptr_t slabs1 = (uintptr_t)slabs;

#pragma unroll 2
			for (; slabs < send; ++slabs) {
				if ((uintptr_t)slabs > slabs1)
					sti_ = *slabs;

				sti_skip = (sti_ >> 10);
				sti_general += (sti_ & 1023) << mip_lvl;
				if (sti_skip == 0)
					continue;

				int texture = tex;
				tex += sti_skip;
				sti_skip <<= mip_lvl;

				float sti_general_sti_skip = sti_general;
				sti_general += sti_skip;

				float correct_zz1 = pos3d_z;
				float correct_yy1 = pos3d_y;
				if (viewpos.y + sti_general_sti_skip >= 0) {
					correct_zz1 += corr_zz;
					correct_yy1 += corr_yy;
				}

				pos3d_z1 = correct_zz1 + pos3d_z_add * sti_general_sti_skip;
				if (pos3d_z1 <= 0)
					continue;
				pos3d_y1 = correct_yy1 + pos3d_y_add * sti_general_sti_skip;
				scr_y2 = res_y2 + pos3d_y1 / pos3d_z1;
				if (scr_y2 <= y_clip_min) {
					break;
				}

				float correct_zz2 = pos3d_z;
				float correct_yy2 = pos3d_y;
				if (viewpos.y + sti_general < 0) {
					correct_zz2 += corr_zz;
					correct_yy2 += corr_yy;
				}

				pos3d_z2 = correct_zz2 + pos3d_z_add * sti_general;
				if (pos3d_z2 <= 0)
					continue;
				pos3d_y2 = correct_yy2 + pos3d_y_add * sti_general;
				scr_y1 = res_y2 + pos3d_y2 / pos3d_z2 - 1;
				if (scr_y1 >= y_clip_max)
					continue;

				if (scr_y2 >= y_clip_max) {
					scr_y2 = y_clip_max;
#ifdef FLOATING_HORIZON
					y_clip_max = scr_y1;
#endif
				}
				if (scr_y1 <= y_clip_min) {
					scr_y1 = y_clip_min;
#ifdef FLOATING_HORIZON
					y_clip_min = scr_y2;
#endif
#ifdef SHAREMEMCLIP
	#ifdef XFLOATING_HORIZON
					while ((y_clip_max > y_clip_min)
							&& (y_cache[y_clip_min >> 5] & (1 << (y_clip_min & 31))))
						++y_clip_min;
	#endif
#endif
				}
				int y = scr_y1;

#ifdef PERPIXELFORWARD
				y += ofs_skip_start[y];
				// scr_y2-=ofs_skip_start[scr_y2-1]>>16;
	#ifdef XFLOATING_HORIZON
				y_clip_min += ofs_skip_start[y_clip_min];
	#endif
				if (y >= scr_y2)
					continue;
#endif

#ifdef SHAREMEMCLIP
				while ((y < scr_y2) && (y_cache[y >> 5] & (1 << (y & 31))))
					++y;
				if (y >= scr_y2)
					continue;
#endif
				float scr_y1r, pos3d_z2r, pos3d_y2r;
				float scr_y2r, pos3d_z1r, pos3d_y1r;

				pos3d_z1r = pos3d_z + pos3d_z_add * sti_general_sti_skip;
				pos3d_y1r = pos3d_y + pos3d_y_add * sti_general_sti_skip;

				pos3d_z2r = pos3d_z + pos3d_z_add * sti_general;
				pos3d_y2r = pos3d_y + pos3d_y_add * sti_general;

				scr_y2r = res_y2 + pos3d_y1r / pos3d_z1r;
				scr_y1r = res_y2 + pos3d_y2r / pos3d_z2r;

				float u1z = tex / pos3d_z2r;
				float u2dz = texture / pos3d_z1r - u1z;
				float onez1 = 1 / pos3d_z2r;
				float onedz2 = 1 / pos3d_z1r - onez1;

				u2dz /= scr_y2r - scr_y1r;
				onedz2 /= scr_y2r - scr_y1r;

#ifdef PERPIXELFORWARD
				int skip_add = ofs_skip_start[scr_y2 - 1];
				int iskip_add = ofs_skip_start[scr_y1];
				int sa = 0;
#endif

				float mult = y + 1 - scr_y1r;
				float uz = u1z + u2dz * mult;
				float onez = onez1 + onedz2 * mult;

#ifdef PERPIXELFORWARD
				for (; y < scr_y2; uz += u2dz, onez += onedz2)
#else
				for (; y < scr_y2; ++y, uz += u2dz, onez += onedz2)
#endif
				{
#ifdef PERPIXELFORWARD
					int skip = ofs_skip_start[y];

					if (skip) {
						y += skip;
						continue;
					}

					int skip_plus = scr_y2 - 1 - y; //+ skip_add;
					int skip_minus = -scr_y1 + y + iskip_add;
					ofs_skip_start[y] = skip_plus; //+ ( skip_minus<<16 );
#endif
#ifdef SHAREMEMCLIP
					int y5 = y >> 5;
					int y31 = 1 << (y & 31);
					if (y_cache[y5] & y31)
						continue;
#endif
					uint u = min(max(float(uz / onez), float(texture)), float(tex - 1.0));
					uint real_z = int(float(1 / onez)) & 0xfffe;

					uint color16 = send[u];

					((uint*)ofs_rgb_start)[y]
						= color16 + (real_z << 16); //+(real_z<<16);//depth;//send[uint(u)]
#ifdef PERPIXELFORWARD
					++y;
#endif
#ifdef SHAREMEMCLIP
					y_cache[y5] |= y31;
#endif
				} // pixel loop
			} // rle loop
		}
	}
};

void gpu_memcpy(void* dst, void* src, int size)
{
	CUDA_SAFE_CALL(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
	CUT_CHECK_ERROR("cudaMemcpy cudaMemcpyHostToDevice failed");
}

void* gpu_malloc(int size)
{
	void* ptr = 0;

	CUDA_SAFE_CALL(cudaMalloc((void**)&ptr, size));
	CUT_CHECK_ERROR("cudaMalloc failed");

	if (ptr == 0) {
		printf("\ncudaMalloc %d MB: out of memory error\n", (size >> 20));
		while (1) {}
	}

	return ptr;
}

__global__ void cuda_render(
		Render* render_local,
		int maxrays,
		vec3f viewpos,
		vec3f viewrot,
		int res_x,
		int res_y,
		ushort* skipmap_gpu)
{
	extern __shared__ int sdata[];

	int x = (blockIdx.y * 2 + blockIdx.x) * blockDim.x + threadIdx.x;

	if (x >= maxrays)
		return;

	size_t idx = ((x) & (THREAD_COUNT - 1)) * (16300 / (THREAD_COUNT * 4));

	render_local->render_line(
			x,
			(unsigned int*)&sdata[idx],
			viewpos,
			viewrot,
			res_x,
			res_y,
			skipmap_gpu + x * res_y
		);

	return;
}

void cuda_main_render2(int pbo_out, int width, int height, RayMap* raymap)
{
	if (pbo_out == 0)
		return;

	static Render render;

	size_t render_len = sizeof(Render);
	size_t skipmap_len = RAYS_CASTED * RENDER_SIZE * 4;

	static Render* render_gpu = (Render*)((uintptr_t)bmalloc(render_len) + cpu_to_gpu_delta);
	static ushort* skipmap_gpu = (ushort*)((uintptr_t)bmalloc(skipmap_len) + cpu_to_gpu_delta);

	if ((long)render_gpu == cpu_to_gpu_delta) {
		printf("render_gpu == 0\n");
		while (1) {}
	}

	int lines_to_raycast = raymap->map_line_count;
	int thread_calls = ((raymap->map_line_count / 2) | (THREAD_COUNT - 1)) + 1;
	if (lines_to_raycast > RAYS_CASTED)
		lines_to_raycast = RAYS_CASTED;

	int* out_data;
	CUDA_SAFE_CALL(cudaGLMapBufferObject((void**)&out_data, pbo_out));
	if (out_data == 0)
		return;

	dim3 threads(THREAD_COUNT, 1, 1);
	dim3 grid(2, thread_calls / (threads.x), 1);

	render.set_target(width, height, (int*)out_data);
	render.set_raymap(raymap);

	gpu_memcpy(render_gpu, &render, sizeof(Render));

	CUDA_SAFE_CALL(cudaThreadSynchronize());

	cuda_render<<<grid, threads, 16300>>>(
			render_gpu,
			render.ray_map.map_line_count,
			render.ray_map.position,
			render.ray_map.rotation,
			render.res_x,
			render.res_y,
			skipmap_gpu);

	CUT_CHECK_ERROR("cudaRender failed");

	CUDA_SAFE_CALL(cudaThreadSynchronize());

	CUDA_SAFE_CALL(cudaGLUnmapBufferObject(pbo_out));
}

void cuda_pbo_register(int pbo)
{
	CUDA_SAFE_CALL(cudaGLRegisterBufferObject(pbo));
	CUT_CHECK_ERROR("cudaGLRegisterBufferObject failed");
}

void cuda_pbo_unregister(int pbo)
{
	CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(pbo));
	CUT_CHECK_ERROR("cudaGLUnregisterBufferObject failed");
}
