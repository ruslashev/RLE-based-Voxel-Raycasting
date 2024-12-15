#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D texDecal;
uniform float rot_x_gt0;
uniform vec2 vp;
uniform vec4 ofs_add;

uniform vec4 res_x_y_ray_ratio;

void main(void)
{
	float RESX   = res_x_y_ray_ratio.x;
	float RESY   = res_x_y_ray_ratio.y;
	float MAXRAY = res_x_y_ray_ratio.z;

	float xs = gl_FragCoord.x / RESX;
	float ys = gl_FragCoord.y / RESY;

	float border = (RESX - RESY) / (RESX * 2.);

	float scx1 = xs - vp.x;
	float scy1 = ys - vp.y;

	float upper = float(ys < vp.y);
	float left = float(xs < vp.x);
	float wide = float(abs(xs - vp.x) * RESX > abs(ys - vp.y) * RESY);

	float seg_up = (1. - upper) * (1. - wide);
	float seg_dn =       upper  * (1. - wide);
	float seg_rt = (1. - left ) *       wide ;
	float seg_lt =       left   *       wide ;

	float o2 = (wide * gl_FragCoord.x + (1. - wide) * gl_FragCoord.y) / RESX;

	float ang2 =
		(xs - vp.x)  * abs(1. - upper - vp.y) / (ys - vp.y) +
		upper        * (1. - vp.x) +
		(1. - upper) * (vp.x);

	float ang3 =
		(ys - vp.y) * abs(1. - left - vp.x) / (xs - vp.x) +
		left        * (1. - vp.y) +
		(1. - left) * (vp.y);

	ang3 = ang3 * RESY / RESX + border;

	float x_pre = ang3 * wide + ang2 * (1. - wide);

	vec2 texpos = vec2(0);

	texpos.y = seg_up * (1. - x_pre + ofs_add.x) +
	           seg_dn * (     x_pre + ofs_add.y) +
	           seg_rt * (1. - x_pre + ofs_add.z) +
	           seg_lt * (     x_pre + ofs_add.w);

	texpos.y *= res_x_y_ray_ratio.w * 0.25;

	float seg_up_x = rot_x_gt0 * seg_up + (1.0 - rot_x_gt0) * seg_dn;
	float seg_dn_x = rot_x_gt0 * seg_dn + (1.0 - rot_x_gt0) * seg_up;
	float seg_rt_x = rot_x_gt0 * seg_rt + (1.0 - rot_x_gt0) * seg_lt;
	float seg_lt_x = rot_x_gt0 * seg_lt + (1.0 - rot_x_gt0) * seg_rt;

	texpos.x = seg_up_x * (     o2 + border) +
	           seg_dn_x * (1. - o2 - border) +
	           seg_rt_x * (     o2) +
	           seg_lt_x * (1. - o2);

	vec4 c = texture2D(texDecal, texpos);

	if (c.z != 1.) {
		vec3 light = vec3((1. - c.y) + c.x * 0.3 - 0.5);
		gl_FragColor = vec4(light, 1.);
	} else {
		// sky
		gl_FragColor = vec4(178. / 255., 204. / 255., 1., 1.);
	}
}
