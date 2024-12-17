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

	static void get_error()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			printf("GL Error: %d\n", err);
			printf("Programm Stopped!\n");
			while (1)
				usleep(1000 * 1000);
		}
	}
};

class FBO
{
public:
	enum Type { COLOR = 1, DEPTH = 2 }; // Bits

	int color_tex;
	int color_bpp;
	int depth_tex;
	int depth_bpp;
	Type type;

	int width;
	int height;

	int tmp_viewport[4];

	FBO(int texWidth, int texHeight)
	{
		color_tex = -1;
		depth_tex = -1;
		fbo = -1;
		dbo = -1;
		init(texWidth, texHeight);
	}

	void clear()
	{
		if (color_tex != -1) {
			// destroy objects
			glDeleteRenderbuffers(1, &dbo);
			glDeleteTextures(1, (GLuint*)&color_tex);
			glDeleteTextures(1, (GLuint*)&depth_tex);
			glDeleteFramebuffers(1, &fbo);
		}
	}

	void enable()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glBindRenderbuffer(GL_RENDERBUFFER, dbo);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);

		glGetIntegerv(GL_VIEWPORT, tmp_viewport);
		glViewport(0, 0, width, height);
	}

	void disable()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glViewport(tmp_viewport[0], tmp_viewport[1], tmp_viewport[2], tmp_viewport[3]);
	}

	void init(int texWidth, int texHeight)
	{
		this->width = texWidth;
		this->height = texHeight;

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		get_error();

		// init texture
		glGenTextures(1, (GLuint*)&color_tex);
		glBindTexture(GL_TEXTURE_2D, color_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		get_error();

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
		get_error();

		glGenRenderbuffers(1, &dbo);
		glBindRenderbuffer(GL_RENDERBUFFER, dbo);

		glGenTextures(1, (GLuint*)&depth_tex);
		glBindTexture(GL_TEXTURE_2D, depth_tex);
		get_error();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, texWidth, texHeight, 0,
				GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
		get_error();

		// don't leave this texture bound or fbo (zero) will use it as src,
		// want to use it just as dest GL_DEPTH_ATTACHMENT
		glBindTexture(GL_TEXTURE_2D, 0);
		get_error();

		check_framebuffer_status();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

private:
	GLuint fbo; // frame buffer object ref
	GLuint dbo; // depth buffer object ref

	void get_error()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			printf("GL FBO Error: %d\n", err);
			printf("Programm Stopped!\n");
			while (1) {}
		}
	}

	void check_framebuffer_status()
	{
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status) {
		case GL_FRAMEBUFFER_COMPLETE:
			return;
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("Unsupported framebuffer format\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("Framebuffer incomplete, missing attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			printf("Framebuffer incomplete, attached images must have same dimensions\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			printf("Framebuffer incomplete, attached images must have same format\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			printf("Framebuffer incomplete, missing draw buffer\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("Framebuffer incomplete, missing read buffer\n");
			break;
		case 0:
			printf("Not ok but trying...\n");
			return;
		default:
			printf("Framebuffer error code %d\n", status);
			break;
		};

		printf("Programm Stopped!\n");
		while (1)
			usleep(100 * 1000);
	}
};

extern GL_Main gl_main;
