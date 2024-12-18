uniform sampler2D texDecal;
uniform float lookdown;
uniform vec2 vp;
uniform vec4 ofs_add;
uniform vec2 res;

void main(void)
{
	float xs = gl_FragCoord.x / res.x;
	float ys = gl_FragCoord.y / res.y;
	float xn = xs - vp.x;
	float yn = ys - vp.y;
	float ratio = res.y / res.x;

	float border = (1. - ratio) / 2.;

	float high = float(yn < 0.);
	float left = float(xn < 0.);
	float wide = float(abs(xn) * res.x > abs(yn) * res.y);

	float seg_up = (1. - high) * (1. - wide);
	float seg_dn =       high  * (1. - wide);
	float seg_rt = (1. - left) *       wide ;
	float seg_lt =       left  *       wide ;

	float nseg_up = lookdown * seg_up + (1. - lookdown) * seg_dn;
	float nseg_dn = lookdown * seg_dn + (1. - lookdown) * seg_up;
	float nseg_rt = lookdown * seg_rt + (1. - lookdown) * seg_lt;
	float nseg_lt = lookdown * seg_lt + (1. - lookdown) * seg_rt;

	float ang_vert =
		xn          * abs(1. - high - vp.y) / yn +
		high        * (1. - vp.x) +
		(1. - high) * vp.x;

	float ang_horz =
		yn          * abs(1. - left - vp.x) / xn +
		left        * (1. - vp.y) +
		(1. - left) * vp.y;

	ang_horz = ang_horz * ratio + border;

	vec2 texpos;

	texpos.y = seg_up * (1. - ang_vert + ofs_add.x) +
	           seg_dn * (     ang_vert + ofs_add.y) +
	           seg_rt * (1. - ang_horz + ofs_add.z) +
	           seg_lt * (     ang_horz + ofs_add.w);

	texpos.y *= 0.25;

	float overt = ys * ratio + border;
	float ohorz = xs;

	texpos.x = nseg_up * (     overt) +
	           nseg_dn * (1. - overt) +
	           nseg_rt * (     ohorz) +
	           nseg_lt * (1. - ohorz);

	vec4 c = texture2D(texDecal, texpos);

	if (c.z != 1.) {
		vec3 light = vec3((1. - c.y) + c.x * 0.3 - 0.5);
		gl_FragColor = vec4(light, 1.);
	} else {
		// sky
		gl_FragColor = vec4(178. / 255., 204. / 255., 1., 1.);
	}
}
