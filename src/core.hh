#pragma once

#include <cstdint>

#define SCREEN_SIZE_X 1024
#define SCREEN_SIZE_Y 768
#define RENDER_SIZE 1024
#define RAYS_CASTED (SCREEN_SIZE_X*4)
#define RAYS_CASTED_RES (SCREEN_SIZE_X*4)
#define RAYS_DISTANCE 80000
#define MIP_DISTANCE (SCREEN_SIZE_X)
#define THREAD_COUNT 128

// #define MEM_TEXTURE

// #define ANTIALIAS
// #define CLIPREGION

// #define NO_ROTATION

#define FLOATING_HORIZON
#define XFLOATING_HORIZON

// clipping
// #define PERPIXELFORWARD
#define SHAREMEMCLIP

#define loop(a_l,start_l,end_l) for ( a_l = start_l;a_l<end_l;++a_l )

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned char uchar;

struct Keyboard
{
	bool key [256]; // actual
	bool key2[256]; // before

	Keyboard()
	{
		int a;
		loop(a, 0, 256)
			key[a] = key2[a] = 0;
	}

	bool KeyDn(char a)
	{
		return key[a];
	}

	bool KeyPr(char a)
	{
		return ((!key2[a]) && key[a] );
	}

	bool KeyUp(char a)
	{
		return ((!key[a]) && key2[a] );
	}

	void update()
	{
		int a;
		loop(a, 0, 256)
			key2[a] = key[a];
	}
};

struct Mouse
{
	bool  button[256];
	bool  button2[256];
	float mouseX, mouseY;
	float mouseDX, mouseDY;

	Mouse()
	{
		int a;
		loop(a, 0, 256)
			button[a] = button2[a] = 0;
		mouseX = mouseY = mouseDX = mouseDY =  0;
	}

	void update()
	{
		int a;
		loop(a, 0, 256)
			button2[a] = button[a];
	}
};

struct Screen
{
	int  window_width;
	int  window_height;
	bool fullscreen;

	float posx, posy, posz;
	float rotx, roty, rotz;
};

extern Keyboard keyboard;
extern Mouse    mouse;
extern Screen   screen;

struct RayMap_GPU;

extern "C" {
    extern void  cpu_memcpy(void* dst, void* src, int count);
    extern void  gpu_memcpy(void* dst, void* src, int count);
    extern void* gpu_malloc(int size);
    extern void  cuda_create_1d_texture(char* data32, int size);
    extern void  cuda_create_2d_texture(uint* data64, int width,int height);
    extern void  cuda_pbo_register(int pbo);
    extern void  cuda_pbo_unregister(int pbo);
    extern void  cuda_main_render2(int pbo_out, int width, int height, RayMap_GPU* raymap);

    extern intptr_t cpu_to_gpu_delta;
};
