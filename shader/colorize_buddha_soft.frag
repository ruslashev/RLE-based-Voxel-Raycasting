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

	// float upper = step(scy1, 0.);
	float upper;
	if (ys < vp.y) {
		upper = 1.;
	} else {
		upper = 0.;
	}

	// float left = step(scx1, 0.);
	float left;
	if (xs < vp.x) {
		left = 1.;
	} else {
		left = 0.;
	}

	// float wide = step(abs(scy1) - abs(scx1) * RESX / RESY, 0.);
	float wide;
	if (abs(xs - vp.x) * RESX > abs(ys - vp.y) * RESY) {
		wide = 1.;
	} else {
		wide = 0.;
	}

	float seg_up = (1. - upper) * (1. - wide);
	float seg_dn =       upper  * (1. - wide);
	float seg_rt = (1. - left ) *       wide ;
	float seg_lt =       left   *       wide ;

	float o2 = (wide * gl_FragCoord.x + (1. - wide) * gl_FragCoord.y) / RESX;

	float ang2 =
		scx1 * abs(1. - step(scy1, 0.) - vp.y) / scy1 +
		step(scy1, 0.)        * (1. - vp.x) +
		(1. - step(scy1, 0.)) * (vp.x);

	float ang3 =
		scy1 * abs(1. - step(scx1, 0.) - vp.x) / scx1 +
		step(scx1, 0.)        * (1. - vp.y) +
		(1. - step(scx1, 0.)) * (vp.y);

	ang3 = ang3 * RESY / RESX + border;

	float x_pre = (wide * ang3 + ang2 * (1. - wide));

	vec2 texpos = vec2(0);

	texpos.y = seg_dn * (ofs_add.y +      x_pre) +
	           seg_up * (ofs_add.x + 1. - x_pre) +
	           seg_lt * (ofs_add.w +      x_pre) +
	           seg_rt * (ofs_add.z + 1. - x_pre);

	texpos.y *= res_x_y_ray_ratio.w * 0.25;

	float seg_up_x = rot_x_gt0 * seg_up + (1.0 - rot_x_gt0) * seg_dn;
	float seg_dn_x = rot_x_gt0 * seg_dn + (1.0 - rot_x_gt0) * seg_up;
	float seg_rt_x = rot_x_gt0 * seg_rt + (1.0 - rot_x_gt0) * seg_lt;
	float seg_lt_x = rot_x_gt0 * seg_lt + (1.0 - rot_x_gt0) * seg_rt;

	texpos.x = seg_up_x * (      o2 + border) +
	           seg_dn_x * (1. - (o2 + border)) +
	           seg_rt_x * (      o2) +
	           seg_lt_x * (1. -  o2);

	vec4 c_out = vec4(0.);
	float fragz = 0.;

	vec4 c = texture2D(texDecal, texpos);

	if (c.z != 1.) {
		float z = (c.z * (1. / 256.) + c.w);
		fragz = 0.001 / z;
		float pos3dx = z * (xs * 2. - 1.);
		float pos3dy = z * (ys * 2. - 1.);

		vec3 nrm;
		nrm.x = c.r;
		nrm.y = c.g;
		nrm.z = min(1.0, nrm.x + nrm.y);

		nrm = 2.0 * nrm - vec3(1.0);
		nrm.z = sqrt(1. - c.r * c.r - c.g * c.g);
		nrm = normalize(nrm);

		vec3 lgt;
		lgt.x = pos3dx;
		lgt.y = pos3dy;
		lgt.z = z;
		lgt.x = lgt.x - 30.;
		lgt.y = lgt.y + 25.;
		lgt = normalize(lgt);

		float light = (1. - c.g) + c.r * 0.3 - 0.5;

		c.x = light;
		c.y = light;
		c.z = light;

		vec4 pow4 = (max(c, 0.) * max(c, 0.) * max(c, 0.) * max(c, 0.));

		c = c * vec4(1.3, 0.9, 0.7, 1.0) + 1.2 * pow4 * vec4(1.2, 1.2, 1.2, 1.0);
	} else {
		// sky
		c = vec4(178. / 255., 204. / 255., 1., 1.);
	}

	c_out += c;

	c_out.w = fragz;
	gl_FragColor = c_out;
}
