#include "ray_map.hh"

RayMap::RayMap()
{
	map_line_limit = 2500;
	border = 0;
	clip_min = border;
	clip_max = 1 - clip_min;
}

void RayMap::set_border(float a)
{
	border = a;
	clip_min = border;
	clip_max = 1 - clip_min;
}

void RayMap::set_ray_limit(int a)
{
	map_line_limit = a;
}

static inline float angle(const vec3f& a, const vec3f& b)
{
	float dot = a.x * b.x + a.y * b.y + a.z * b.z;
	float len = a.len() * b.len();
	if (len == 0) len = 0.00001f;
	float input = dot / len;
	if (input < -1) input = -1;
	if (input > 1) input = 1;
	return (float)acos(input);
}

void RayMap::get_ray_map(vec3f pos, vec3f rot)
{
	// Define frustum & origin
	rotation = rot;
	position = pos;

	vec3f p[6] = {
		vec3f( 1,  1, 1), // [0] frustum
		vec3f(-1,  1, 1), // [1]    "
		vec3f(-1, -1, 1), // [2]    "
		vec3f( 1, -1, 1), // [3]    "
		vec3f(0, 0, 0),   // [4] origin
		vec3f(0, 0, 0)    // [5] intersection frustum/vec(0,1,0); will be calculated
	};

	matrix44 m;
	m.ident();
	m.rotate_x(rotation.x);
	m.rotate_y(rotation.y);

	// Transform frustum
	for (int i = 0; i < 5; i++)
		p[i] = m * p[i];

	// Calculate vanishing point p[5]
	vec3f down(0, -1, 0);
	vec3f view;
	view = (p[0] + p[2]) / 2 - p[4];
	float ang   = angle(down, view);
	float alpha = float(M_PI) / 2 - ang;
	float scale = 1 / sin(alpha);

	p[5] = p[4] + down * scale;

	// Calc. matrix to transform frustum into 2D (xy-plane)

	matrix44 to2d;
	vec3f nrm = (p[1] - p[0]) * (p[3] - p[0]);

	vec3f d1 = p[1] - p[0];
	vec3f d2 = p[3] - p[0];
	vec3f d3 = nrm;
	vec3f d4 = p[0];

	d1 *= 1 / d1.dot(d1);
	d2 *= 1 / d2.dot(d2);
	d3 *= 1 / d3.dot(d3);

	to2d.ident();
	to2d.set(d1, d2, d3, d4);
	to3d = to2d;
	to2d.invert_simpler();

	// Transform 3D to 2D

	vp = to2d * p[5];
	vp.z = 0;

	// Calc Lines

	int visible_rays = 0;
	int safety = 2;
	float ys_min = border;
	float ys_max = 1 - border;

	map_line_count = 0;
	maxres = RAYS_CASTED_RES / 4; // RAYS_CASTED/4;

	vec3f plist[4] = {
		vec3f(-1, -1, 0),
		vec3f( 1, -1, 0),
		vec3f( 1,  1, 0),
		vec3f(-1,  1, 0)
	};

	vec3f plist2[4] = {
		vec3f(0, ys_min, 0),
		vec3f(1, ys_min, 0),
		vec3f(1, ys_max, 0),
		vec3f(0, ys_max, 0)
	};

	res[0] = 0;
	res[1] = 0;
	res[2] = 0;
	res[3] = 0;

	// Upper Part
	if (vp.y > border) {
		p_no[0] = vp + plist[0] * abs(vp.y - border);
		p_no[1] = vp + plist[1] * abs(vp.y - border);

		vec3f p_no_in = p_no[1];

		if (vp.x > 1) {
			if (p_no[1].x > 1)
				p_no[1].x = 1;
		} else if ((p_no[0] - vp).dot(plist2[2] - vp) > 0) {
			p_no[1] = vp
				+ (plist2[2] - vp)
				* abs((plist2[0].y - vp.y) / (plist2[2].y - vp.y));
		}
		if (vp.x < 0) {
			if (p_no[0].x < 0)
				p_no[0].x = 0;
		} else if ((p_no_in - vp).dot(plist2[3] - vp) > 0) {
			p_no[0] = vp
				+ (plist2[3] - vp)
				* abs((plist2[1].y - vp.y) / (plist2[3].y - vp.y));
		}
		p_no[0].y = p_no[1].y = plist2[0].y;
		p_no[0].x = float(int(maxres * p_no[0].x) - safety) / maxres;
		p_no[1].x = float(int(maxres * p_no[1].x) + safety) / maxres;
		res[0] = maxres * abs(p_no[0].x - p_no[1].x);

		if (p_no[0].x - p_no[1].x > 0)
			res[0] = 0;
		if (res[0] > (maxres * 3))
			res[0] = (maxres * 3);

		p_ofs_min[0] = p_no[0].x;
	}

	// Lower Part
	if (vp.y < 1 - border) {
		p_no[2] = vp + plist[3] * abs(vp.y - 1 + border);
		p_no[3] = vp + plist[2] * abs(vp.y - 1 + border);

		vec3f p_no_in = p_no[2];

		if (vp.x < 0) {
			if (p_no[2].x < 0)
				p_no[2].x = 0;
		} else if ((p_no[3] - vp).dot(plist2[0] - vp) > 0) {
			p_no[2] = vp
				+ (plist2[0] - vp)
				* abs((plist2[2].y - vp.y) / (plist2[0].y - vp.y));
		}

		if (vp.x > 1) {
			if (p_no[3].x > 1)
				p_no[3].x = 1;
		} else {
			vec3f delta = plist2[1] - vp;
			if ((p_no_in - vp).dot(delta) > 0)
				p_no[3] = vp + delta * abs((plist2[3].y - vp.y) / delta.y);
		}

		p_no[2].y = p_no[3].y = plist2[2].y;
		p_no[2].x = float(int(maxres * p_no[2].x) - safety) / maxres;
		p_no[3].x = float(int(maxres * p_no[3].x) + safety) / maxres;
		res[1] = maxres * abs(p_no[2].x - p_no[3].x);

		if (p_no[2].x - p_no[3].x > 0)
			res[1] = 0;
		if (res[1] > (maxres * 3))
			res[1] = (maxres * 3);

		p_ofs_min[1] = p_no[2].x;
	}

	// Left Part
	if (vp.x > 0) {
		p_no[4] = vp + plist[0] * abs(vp.x); // -1 -1
		p_no[5] = vp + plist[3] * abs(vp.x); // -1  1

		vec3f p_no_in = p_no[5];

		if (vp.y > 1 - border) {
			if (p_no[5].y > 1 - border)
				p_no[5].y = 1 - border;
		} else {
			int id = 2;
			if ((p_no[4] - vp).dot(plist2[id] - vp) > 0) {
				p_no[5] = vp
					+ (plist2[id] - vp)
					* abs((plist2[id ^ 1].x - vp.x) / (plist2[id].x - vp.x));
			}
		}
		if (vp.y < border) {
			if (p_no[4].y < border)
				p_no[4].y = border;
		} else {
			int id = 1;
			if ((p_no_in - vp).dot(plist2[id] - vp) > 0) {
				p_no[4] = vp
					+ (plist2[id] - vp)
					* abs((plist2[id ^ 1].x - vp.x) / (plist2[id].x - vp.x));
			}
		}
		p_no[4].x = p_no[5].x = 0;
		p_no[4].y = float(int(maxres * p_no[4].y) - safety) / maxres;
		p_no[5].y = float(int(maxres * p_no[5].y) + safety) / maxres;
		res[2] = maxres * abs(p_no[4].y - p_no[5].y);

		if (p_no[4].y - p_no[5].y > 0)
			res[2] = 0;
		if (res[2] > (maxres * 3))
			res[2] = (maxres * 3);

		p_ofs_min[2] = p_no[4].y;
	}

	// Right Part
	if (vp.x < 1) {
		p_no[6] = vp + plist[1] * abs(1 - vp.x); // 1 -1
		p_no[7] = vp + plist[2] * abs(1 - vp.x); // 1  1

		vec3f p_no_in = p_no[7];

		if (vp.y > 1 - border) {
			if (p_no[7].y > 1 - border)
				p_no[7].y = 1 - border;
		} else {

			int id = 3;
			if ((p_no[6] - vp).dot(plist2[id] - vp) > 0) {
				p_no[7] = vp
					+ (plist2[id] - vp)
					* abs((plist2[id ^ 1].x - vp.x) / (plist2[id].x - vp.x));
			}
		}
		if (vp.y < border) {
			if (p_no[6].y < border)
				p_no[6].y = border;
		} else {
			int id = 0;
			if ((p_no_in - vp).dot(plist2[id] - vp) > 0) {
				p_no[6] = vp
					+ (plist2[id] - vp)
					* abs((plist2[id ^ 1].x - vp.x) / (plist2[id].x - vp.x));
			}
		}

		p_no[6].x = p_no[7].x = 1;
		p_no[6].y = float(int(maxres * p_no[6].y) - safety) / maxres;
		p_no[7].y = float(int(maxres * p_no[7].y) + safety) / maxres;
		res[3] = maxres * abs(p_no[6].y - p_no[7].y);

		if (p_no[6].y - p_no[7].y > 0)
			res[3] = 0;
		if (res[3] > (maxres * 3))
			res[3] = (maxres * 3);

		p_ofs_min[3] = p_no[6].y;
	}

	visible_rays = res[0] + res[1] + res[2] + res[3];

	map_line_count = visible_rays;
}
