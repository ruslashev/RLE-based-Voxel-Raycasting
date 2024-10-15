#include "vec_math.hh"

vec3f vec3f::normalize(vec3f a)
{
	float square = a.x * a.x + a.y * a.y + a.z * a.z;

	if (square <= 0.00001f) {
		a.x = 1;
		a.y = 0;
		a.z = 0;
		return a;
	}

	float len = 1.f / (float)sqrt(square);

	a.x *= len;
	a.y *= len;
	a.z *= len;

	return a;
}
