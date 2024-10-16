#pragma once

#include "../inc/mathlib/vector.h"

struct vec3f
{
	float x, y, z;

	inline vec3f(void)
	{}

	inline vec3f(vector3 a)
	{ x = a.x; y = a.y; z = a.z; }

	inline vec3f(const float X, const float Y, const float Z)
	{ x = X; y = Y; z = Z; }

	inline vec3f operator + (const vec3f& a) const
	{ return vec3f(x + a.x, y + a.y, z + a.z); }

	inline vec3f operator += (const vec3f& a) const
	{ return vec3f(x + a.x, y + a.y, z + a.z); }

	inline vec3f operator * (const float a) const
	{ return vec3f(x * a, y * a, z * a); }

	inline vec3f operator * (const vec3f a) const
	{ return vec3f(x * a.x, y * a.y, z * a.z); }

	inline vector3 v3() const
	{ return vector3(x , y, z); }

	inline vec3f operator = (const vector3 a)
	{ x = a.x; y = a.y; z = a.z; return *this; }

	inline vec3f operator = (const vec3f a)
	{ x = a.x; y = a.y; z = a.z; return *this; }

	inline vec3f operator / (const vec3f a) const
	{ return vec3f(x / a.x, y / a.y, z / a.z); }

	inline vec3f operator - (const vec3f& a) const
	{ return vec3f(x - a.x, y - a.y, z - a.z); }

	inline vec3f operator / (const float a) const
	{ return vec3f(x / a, y / a, z / a); }

	inline float dot(const vec3f& a) const
	{ return a.x * x + a.y * y + a.z * z; }

	inline vec3f cross(const vec3f& a, const vec3f& b)
	{
		x = a.y * b.z - a.z * b.y;
		y = a.z * b.x - a.x * b.z;
		z = a.x * b.y - a.y * b.x;
		return *this;
	}

	inline float angle(const vec3f& v)
	{
		vec3f a = v, b = *this;
		float dot = v.x * x + v.y * y + v.z * z;
		float len = a.length() * b.length();
		if (len == 0) len = 0.00001f;
		float input = dot / len;
		if (input < -1) input = -1;
		if (input > 1) input = 1;
		return (float)acos(input);
	}

	inline float angle2(const vec3f& v, const vec3f& w)
	{
		vec3f a = v, b = *this;
		float dot = a.x * b.x + a.y * b.y + a.z * b.z;
		float len = a.length() * b.length();
		if (len == 0) len = 1;

		vec3f plane;
		plane.cross(b, w);

		if (plane.x * a.x + plane.y * a.y + plane.z * a.z > 0)
			return (float)-acos(dot / len);

		return (float)acos(dot / len);
	}

	inline vec3f rot_x(float a)
	{
		float yy = cos(a) * y + sin(a) * z;
		float zz = cos(a) * z - sin(a) * y;
		y = yy;
		z = zz;
		return *this;
	}

	inline vec3f rot_y(float a)
	{
		float xx = cos(-a) * x + sin(-a) * z;
		float zz = cos(-a) * z - sin(-a) * x;
		x = xx;
		z = zz;
		return *this;
	}

	inline vec3f rot_z(float a)
	{
		float yy = cos(a) * y + sin(a) * x;
		float xx = cos(a) * x - sin(a) * y;
		y = yy;
		x = xx;
		return *this;
	}

	inline void clamp(float min, float max)
	{
		if (x < min) x = min;
		if (y < min) y = min;
		if (z < min) z = min;
		if (x > max) x = max;
		if (y > max) y = max;
		if (z > max) z = max;
	}

	inline vec3f invert()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}

	inline vec3f frac()
	{
		return vec3f(
				x - float(int(x)),
				y - float(int(y)),
				z - float(int(z))
				);
	}

	inline vec3f integer()
	{
		return vec3f(
				float(int(x)),
				float(int(y)),
				float(int(z))
			);
	}

	inline float length() const
	{
		return (float)sqrt(x * x + y * y + z * z);
	}

	inline vec3f normalize(float desired_length = 1)
	{
		float square = x * x + y * y + z * z;
		if (square <= 0.00001f) {
			x = 1;
			y = 0;
			z = 0;
			return *this;
		}
		float len = desired_length / (float)sqrt(square);
		x *= len;
		y *= len;
		z *= len;
		return *this;
	}

	static vec3f normalize(vec3f a);
};
