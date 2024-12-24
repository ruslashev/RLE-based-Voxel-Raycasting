uniform sampler2D texDecal;
uniform vec2 vp;
uniform vec4 ofs_add;
uniform vec2 res;

void main(void)
{
	float ratio = res.y / res.x;
	float border = (1. - ratio) / 2.;

	vec2 vp = 1. - vp;
	vp.y = (vp.y - border) / ratio;

	float xs = gl_FragCoord.x / res.x;
	float ys = gl_FragCoord.y / res.y;
	float xn = xs - vp.x;
	float yn = ys - vp.y;

	vec2 texpos;

	texpos.x = ys * ratio + border;

	texpos.y = 1. - xn / yn * abs(1. - vp.y) - vp.x;
	texpos.y += ofs_add.x;
	texpos.y /= 4.;

	vec4 c = texture2D(texDecal, texpos);

	if ((1. - float(yn < 0.)) * (1. - float(abs(xn) * res.x > abs(yn) * res.y)) == 0.) {
		gl_FragColor = vec4(0.);
		return;
	}

	if (c.z != 1.) {
		vec3 light = vec3((1. - c.y) + c.x * 0.3 - 0.5);
		gl_FragColor = vec4(light, 1.);
	} else {
		// sky
		gl_FragColor = vec4(178. / 255., 204. / 255., 1., 1.);
	}
}
