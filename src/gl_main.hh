#pragma once

#include "glsl.hh"
#include <cstdio>
#include <unistd.h>

struct GL_Main
{
	enum TexLoadFlags {
		TEX_ADDALPHA = 1,
		TEX_NORMALMAP = 2,
		TEX_HORIZONMAP = 4,
		TEX_HORIZONLOOKUP = 8,
		TEX_NORMALIZE = 16,
		TEX_16BIT = 32,
	};

	static GL_Main* This;

	bool fullscreen;

	GL_Main() { This = (GL_Main*)this; }

	// Init
	void Init(int window_width, int window_height, bool fullscreen, void (*display_func)(void));
	void ToggleFullscreen();

	// GLUT Keyboard & Mouse IO
	static void keyDown1Static(int key, int x, int y);
	static void keyDown2Static(unsigned char key, int x, int y);
	static void keyUp1Static(int key, int x, int y);
	static void keyUp2Static(unsigned char key, int x, int y);
	void KeyPressed(int key, int x, int y, bool pressed);
	static void MouseMotionStatic(int x, int y);
	static void MouseButtonStatic(int button, int state, int x, int y);

	// GLUT draw functions
	static void reshape_static(int w, int h);
	static void idle_static();

	// PBO functions
	void deletePBO(GLuint* pbo);
	void createPBO(GLuint* pbo, int image_width, int image_height, int bpp = 32);

	// Texture functions
	void createTexture(GLuint* tex_name, unsigned int size_x, unsigned int size_y, int bpp = 32);
	void deleteTexture(GLuint* tex);
};

extern GL_Main gl_main;
