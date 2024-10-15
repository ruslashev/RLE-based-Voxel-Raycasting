#include <GL/glew.h>

#include "alloc.hh"
#include "bitmap_fonts.h"
#include "gl_main.hh"
#include "glsl.hh"
#include "ray_map.hh"
#include "rle4.hh"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RayMap   ray_map;
Keyboard keyboard;
Mouse    mouse;
Screen   screen;
intptr_t cpu_to_gpu_delta;

static void display();

static glShaderManager shader_manager;
static int rndtex = -1;
static GLuint tex_screen = -2;
static GLuint pbo_dest = -2;
static int cuda_time = 0;

static void* alligned_alloc_zeroed_cpu(size_t sz)
{
	void* ptr = malloc(sz + 16);

	if (!ptr) {
		printf("failed to alloc %zu B\n", sz);
		exit(1);
	}

	memset(ptr, 0, sz + 16);

	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned = (addr + 16) & 0xfffffffffffffff0;

	return (void*)aligned;
}

static void* aligned_alloc_gpu(size_t sz)
{
	void* ptr = gpu_malloc(sz + 16);
	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned = (addr + 16) & 0xfffffffffffffff0;

	return (void*)aligned;
}

int main(int argc, char** argv)
{
	int pool_size = 300 * 1024 * 1024;
	char* pool     = (char*)alligned_alloc_zeroed_cpu(pool_size);
	char* pool_gpu = (char*)aligned_alloc_gpu(pool_size);
	cpu_to_gpu_delta = ((intptr_t)pool_gpu - (intptr_t)pool) & 0xfffffffffffffff0;

	arena_init(pool, pool_size);

	printf("pool: %p\n", pool);

	RLE4 rle4;

	/*
	   rle4.loadvxl("../untitled.vxl");
	   rle4.save("voxelstein.rle4");
	   return 0;
	   */

	printf("Creating Scene \r");

	rle4.init();

	if (!rle4.load("Imrodh.rle4"))
		return 0;

	/*
	 * ushort* vxl = rle4.uncompress(rle4.map4);
	 * int m4sx = rle4.map4.sx/2;
	 * int m4sy = rle4.map4.sy/2;
	 * int m4sz = rle4.map4.sz/2;
	 * rle4.clear();
	 * Map4 m4 = rle4.compress_mip(vxl,m4sx,m4sy,m4sz);
	 * rle4.map4=m4;
	 */

	printf("loading ready.\n");
	rle4.all_to_gpu();

#ifdef MEM_TEXTURE
	rle4.all_to_gpu_tex();
#endif

	printf("copy to gpu ready.\n");
	memcpy(ray_map.map4_gpu, rle4.mapgpu, 10 * sizeof(Map4));
	ray_map.nummaps = rle4.nummaps;

	printf("ready.\n");

	screen.posx = 14447;
	screen.posz = 15021;
	screen.posy = -438;

#ifdef VOXELSTEIN
	screen.posx = 0 * 4414;
	screen.posy = -185;
	screen.posz = 0 * 4924;

	screen.posx = 338;
	screen.posy = -191;
	screen.posz = 109;
#endif

#ifdef BUDDHA
	screen.posx = 499;
	screen.posy = -818;
	screen.posz = 941;
#endif

#ifdef CLIPREGION
	screen.posx = 0;
	screen.posz = 0;
#endif

#ifdef WONDERLAND
	screen.posx = 13522;
	screen.posz = 15892;
	screen.posy = -68; // won
	screen.posy = -619; // won
#endif

#ifdef CLIPREGION
	screen.posx = -632;
	screen.posz = 512;
#endif
#ifndef CLIPREGION
	screen.posx = 10000;
	screen.posz = 10000;
#endif

	gl_main.Init(SCREEN_SIZE_X, SCREEN_SIZE_Y, false, display);
	printf("Starting..\n");

	glutMainLoop();
}

static void update_viewpoint()
{
	int delta = 16;

	static float multiplier = 0.125f;
	float step = float(delta) * multiplier;

	screen.rotx = screen.rotx * 0.9 + 0.1 * ((mouse.mouseY - 0.5) * 10 + 0.01);
	screen.roty = screen.roty * 0.9 + 0.1 * ((mouse.mouseX - 0.5) * 10 + 0.01 + M_PI / 2);

#ifdef NO_ROTATION
	screen.rotx = 0.01;
	screen.roty = 0.01 + M_PI / 2;
#endif

	if (screen.rotx > M_PI - 0.01)
		screen.rotx = M_PI - 0.01;
	if (screen.rotx < -M_PI + 0.01)
		screen.rotx = -M_PI + 0.01;

	// Direction matrix
	matrix44 m;
	m.ident();
	m.rotate_z(-screen.rotz);
	m.rotate_x(-screen.rotx);
	m.rotate_y(-screen.roty);

	// Transform direction vector
	static vec3f pos(screen.posx, screen.posy, screen.posz);
	vec3f forward = m * vec3f(0, 0, -step).v3();
	vec3f side    = m * vec3f(-step, 0, 0).v3();
	vec3f updown  = m * vec3f(0, -step, 0).v3();

	if (keyboard.KeyDn(119)) pos = pos + forward;
	if (keyboard.KeyDn(115)) pos = pos - forward;
	if (keyboard.KeyDn(97)) pos = pos + side;
	if (keyboard.KeyDn(100)) pos = pos - side;
	if (keyboard.KeyDn(113)) pos = pos + updown;
	if (keyboard.KeyDn(101)) pos = pos - updown;
	if (keyboard.KeyPr(102)) gl_main.ToggleFullscreen();
	if (keyboard.KeyPr(43)) multiplier *= 2;
	if (keyboard.KeyPr(45)) if (multiplier > 0.01) multiplier /= 2;

	screen.posx = screen.posx * 0.9 + 0.1 * pos.x;
	screen.posy = screen.posy * 0.9 + 0.1 * pos.y;
	screen.posz = screen.posz * 0.9 + 0.1 * pos.z;
}

static void render_to_pbo()
{
	static int render_width = RENDER_SIZE;
	static int render_height = RENDER_SIZE;

	if (pbo_dest == -2) {
		gl_main.createPBO(&pbo_dest, RENDER_SIZE, RAYS_CASTED, 32);
		gl_main.createTexture(&tex_screen, RENDER_SIZE, RAYS_CASTED, 32);
	}

	// run the Cuda kernel
	cuda_main_render2(pbo_dest, render_width, render_height, &ray_map);

	// blit convolved texture onto the screen

	// download texture from PBO
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_dest);
	glBindTexture(GL_TEXTURE_2D, tex_screen);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RENDER_SIZE, RAYS_CASTED, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void display_pbo()
{
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	static glShader* shader_soft = 0;
	static glShader* shader_colorize = 0;

	glFlush();

	if (!shader_soft)
		shader_soft = shader_manager.loadfromFile("shader/soft.vert", "shader/soft.frag");

	if (!shader_colorize) {
		shader_colorize = shader_manager.loadfromFile(
#ifdef ANTIALIAS
	#ifdef VOXELSTEIN
				"shader/colorize_stein_soft_2xAA.vert", "shader/colorize_stein_soft_2xAA.frag"
	#endif
	#ifdef WONDERLAND
				"shader/colorize_soft_2xAA.vert", "shader/colorize_soft_2xAA.frag"
	#endif
	#ifdef BUDDHA
				"shader/colorize_buddha_soft_2xAA.vert", "shader/colorize_buddha_soft_2xAA.frag"
	#endif
#else
	#ifdef VOXELSTEIN
				"shader/colorize_stein_soft.vert", "shader/colorize_stein_soft.frag"
	#endif
	#ifdef WONDERLAND
				"shader/colorize_soft.vert", "shader/colorize_soft.frag"
	#endif
	#ifdef BUDDHA
				"shader/colorize_buddha_soft.vert", "shader/colorize_buddha_soft.frag"
	#endif
#endif
		);
	}

	static FBO fbo1(2048, 2048);

	FBO* fbo = &fbo1;

	fbo->enable();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	glBindTexture(GL_TEXTURE_2D, tex_screen);

	// render a screen sized quad
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float border = ray_map.border;

	shader_colorize->begin();

	// Shader Parameters
	shader_colorize->setUniform1i("texDecal", 0);
	shader_colorize->setUniform2f("vanish",
			1 - ray_map.vanishing_point_2d.x,
			(1 - ray_map.vanishing_point_2d.y - border) * float(screen.window_width) / float(screen.window_height));

	float ofs1 = 4 * float(ray_map.res[0]) / float(RAYS_CASTED_RES);
	float ofs2 = 4 * float(ray_map.res[1]) / float(RAYS_CASTED_RES) + ofs1;
	float ofs3 = 4 * float(ray_map.res[2]) / float(RAYS_CASTED_RES) + ofs2;

	shader_colorize->setUniform1f("rot_x_greater_zero", (screen.rotx > 0) ? 1 : 0);

	shader_colorize->setUniform4f("ofs_add",
			-ray_map.p_ofs_min[0],
			-ray_map.p_ofs_min[1] + ofs1,
			-ray_map.p_ofs_min[2] + ofs2,
			-ray_map.p_ofs_min[3] + ofs3);

	shader_colorize->setUniform4f("res_x_y_ray_ratio",
			screen.window_width,
			screen.window_height,
			RAYS_CASTED,
			float(RAYS_CASTED_RES) / float(RAYS_CASTED));

	float vright = 2.0 * float(screen.window_width) / 2048.0 - 1.0;
	float vdown = 2.0 * float(screen.window_height) / 2048.0 - 1.0;
	float tright = 1.0 * float(screen.window_width) / 2048.0;
	float tdown = 1.0 * float(screen.window_height) / 2048.0;

	glDisable(GL_BLEND);
	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, 1);
	glVertex3f(    -1,    -1, 0.5);
	glVertex3f(vright,    -1, 0.5);
	glVertex3f(vright, vdown, 0.5);
	glVertex3f(    -1, vdown, 0.5);
	glEnd();

	shader_colorize->end();

	fbo->disable();

	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, screen.window_width, screen.window_height);

	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rndtex);

	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo->color_tex);

	shader_soft->begin();
	shader_soft->setUniform1i("texDecal", 0);
	glDisable(GL_BLEND);
	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, 1);
	glTexCoord2f(0,      0);     glVertex3f(-1, -1,  1);
	glTexCoord2f(tright, 0);     glVertex3f( 1, -1,  1);
	glTexCoord2f(tright, tdown); glVertex3f( 1,  1,  1);
	glTexCoord2f(0,      tdown); glVertex3f(-1,  1,  1);
	glEnd();
	shader_soft->end();

	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTextureARB(GL_TEXTURE0);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	glFlush();
}

static void compute_ray_map()
{
	vec3f pos(screen.posx, screen.posy, screen.posz);
	vec3f rot(screen.rotx, screen.roty, screen.rotz);

	// ray_map.set_border(0.5 * (float(SCREEN_SIZE_X) / float(SCREEN_SIZE_Y) - 1.0f));
	ray_map.set_border(0.125); // todo sven
	// ray_map.set_border(0.5 * (float(screen.window_width - screen.window_height) / float(screen.window_width)));

	ray_map.set_ray_limit(RAYS_CASTED_RES);
	ray_map.get_ray_map(pos, rot);
}

static float fps_count(int diff)
{
	const int num = 10;

	static int measurements[num] = { 0 };
	static int idx = 0;
	static int sum = 0;

	sum -= measurements[idx];
	sum += diff;
	measurements[idx] = diff;

	idx = (idx + 1) % num;

	return (float)sum / (float)num;
}

static void display()
{
	int render_start = glutGet(GLUT_ELAPSED_TIME);

	update_viewpoint();
	compute_ray_map();

	render_to_pbo();
	glFlush();

	display_pbo();
	glFlush();

	int render_end = glutGet(GLUT_ELAPSED_TIME);

	float ms = fps_count(render_end - render_start);
	float fps = 1000. / ms;

	beginRenderText(screen.window_width, screen.window_height);
	{
		char text[100];
		int HCOL = 0, VCOL = 5;
		glColor3f(1.0f, 1.0f, 1.0f);
		sprintf(text, "Total: %3.2f msec (%3.3f fps)", ms, fps);
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);

		// sprintf(text, "CUDA Time: %d msec", cuda_time);
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);

		sprintf(text, "Rays: %d RenderTarget:%d", ray_map.map_line_count, RENDER_SIZE);
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);

		sprintf(text, "Screen: %dx%d", screen.window_width, screen.window_height);
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);

		sprintf(text, "Pos: %d %d %d", int(ray_map.position.x), int(ray_map.position.y),
				int(ray_map.position.z));
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);

		sprintf(text, "Rot: %2.2f %2.2f %2.2f", (ray_map.rotation.x), (ray_map.rotation.y),
				(ray_map.rotation.z));
		HCOL += 15;
		renderText(VCOL, HCOL, BITMAP_FONT_TYPE_HELVETICA_12, text);
	}
	endRenderText();
	glFlush();

	glutSwapBuffers();
	glFlush();

	mouse.update();
	keyboard.update();
}
