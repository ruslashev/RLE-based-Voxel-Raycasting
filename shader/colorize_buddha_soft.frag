uniform sampler2D texDecal;
uniform float lookdown;
uniform vec2 vp;
uniform vec4 ofs_add;
uniform vec2 res;

void main(void)
{
	float xs = gl_FragCoord.x / res.x;
	float ys = gl_FragCoord.y / res.y;

	float border = (res.x - res.y) / (res.x * 2.);

	float high = float(ys < vp.y);
	float left = float(xs < vp.x);
	float wide = float(abs(xs - vp.x) * res.x > abs(ys - vp.y) * res.y);

	float seg_up = (1. - high) * (1. - wide);
	float seg_dn =       high  * (1. - wide);
	float seg_rt = (1. - left) *       wide ;
	float seg_lt =       left  *       wide ;

	float nseg_up = lookdown * seg_up + (1. - lookdown) * seg_dn;
	float nseg_dn = lookdown * seg_dn + (1. - lookdown) * seg_up;
	float nseg_rt = lookdown * seg_rt + (1. - lookdown) * seg_lt;
	float nseg_lt = lookdown * seg_lt + (1. - lookdown) * seg_rt;

	float o2 = (wide * gl_FragCoord.x + (1. - wide) * gl_FragCoord.y) / res.x;

	float ang2 =
		(xs - vp.x) * abs(1. - high - vp.y) / (ys - vp.y) +
		high        * (1. - vp.x) +
		(1. - high) * vp.x;

	float ang3 =
		(ys - vp.y) * abs(1. - left - vp.x) / (xs - vp.x) +
		left        * (1. - vp.y) +
		(1. - left) * vp.y;

	ang3 = ang3 * res.y / res.x + border;

	float x_pre = ang3 * wide + ang2 * (1. - wide);

	vec2 texpos;

	texpos.y = seg_up * (1. - x_pre + ofs_add.x) +
	           seg_dn * (     x_pre + ofs_add.y) +
	           seg_rt * (1. - x_pre + ofs_add.z) +
	           seg_lt * (     x_pre + ofs_add.w);

	texpos.y *= 0.25;

	texpos.x = nseg_up * (     o2 + border) +
	           nseg_dn * (1. - o2 - border) +
	           nseg_rt * (     o2) +
	           nseg_lt * (1. - o2);

	vec4 c = texture2D(texDecal, texpos);

	if (c.z != 1.) {
		vec3 light = vec3((1. - c.y) + c.x * 0.3 - 0.5);
		gl_FragColor = vec4(light, 1.);
	} else {
		// sky
		gl_FragColor = vec4(178. / 255., 204. / 255., 1., 1.);
	}
}
