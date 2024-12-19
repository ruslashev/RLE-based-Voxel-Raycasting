#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core.hh"
#include "gl_main.hh"

GL_Main gl_main;
GL_Main* GL_Main::This = NULL;

void GL_Main::keyDown1Static(int key, int x, int y)           { GL_Main::This->KeyPressed(key, x, y, true); }
void GL_Main::keyDown2Static(unsigned char key, int x, int y) { GL_Main::This->KeyPressed(key, x, y, true); }
void GL_Main::keyUp1Static(int key, int x, int y)             { GL_Main::This->KeyPressed(key, x, y, false); }
void GL_Main::keyUp2Static(unsigned char key, int x, int y)   { GL_Main::This->KeyPressed(key, x, y, false); }

void GL_Main::MouseMotionStatic(int x, int y)
{
	mouse.mouseX = float(x) / float(screen.window_width);
	mouse.mouseY = float(y) / float(screen.window_height);
}

void GL_Main::MouseButtonStatic(int button_index, int state, int x, int y)
{
	mouse.button[button_index] = (state == GLUT_DOWN) ? true : false;
}

void GL_Main::KeyPressed(int key, int x, int y, bool pressed)
{
	if (key == 27)
		exit(0);

	usleep(10 * 1000);

	keyboard.key[key & 255] = pressed;
}

void GL_Main::ToggleFullscreen()
{
	static int win_width = screen.window_width;
	static int win_height = screen.window_height;

	if (fullscreen) {
		glutReshapeWindow(win_width, win_height);

		screen.window_width = win_width;
		screen.window_height = win_height;
	} else {
		win_width = screen.window_width;
		win_height = screen.window_height;

		glutFullScreen();
	}

	fullscreen = fullscreen ? false : true;
}

#define __glPi 3.14159265358979323846

static void __gluMakeIdentityd(GLdouble m[16])
{
	m[0 + 4 * 0] = 1; m[0 + 4 * 1] = 0; m[0 + 4 * 2] = 0; m[0 + 4 * 3] = 0;
	m[1 + 4 * 0] = 0; m[1 + 4 * 1] = 1; m[1 + 4 * 2] = 0; m[1 + 4 * 3] = 0;
	m[2 + 4 * 0] = 0; m[2 + 4 * 1] = 0; m[2 + 4 * 2] = 1; m[2 + 4 * 3] = 0;
	m[3 + 4 * 0] = 0; m[3 + 4 * 1] = 0; m[3 + 4 * 2] = 0; m[3 + 4 * 3] = 1;
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble m[4][4];
	double sine, cotangent, deltaZ;
	double radians = fovy / 2 * __glPi / 180;

	deltaZ = zFar - zNear;
	sine = sin(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
		return;
	cotangent = cos(radians) / sine;

	__gluMakeIdentityd(&m[0][0]);
	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	m[3][3] = 0;
	glMultMatrixd(&m[0][0]);
}

void GL_Main::Init(int window_width, int window_height, bool fullscreen, void (*display_func)(void))
{
	screen.window_width = window_width;
	screen.window_height = window_height;
	screen.fullscreen = fullscreen;

	int nop = 1;
	const char* headline = "CUDA Voxel Demo";

	glutInit(&nop, (char**)&headline);
	glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);
	glutCreateWindow(headline);

	this->fullscreen = fullscreen;

	if (fullscreen)
		glutFullScreen();

	glewInit();

	glClearColor(0.5, 0.5, 0.5, 1.0);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, window_width, window_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)window_width / (GLfloat)window_height, 0.1, 10.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_LIGHT0);
	float red[] = { 1.0, 0.1, 0.1, 1.0 };
	float white[] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, red);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60.0);

	glutDisplayFunc(display_func);
	glutReshapeFunc(&reshape_static);
	glutIdleFunc(&idle_static);

	glutSpecialFunc(&keyDown1Static);
	glutSpecialUpFunc(&keyUp1Static);
	glutKeyboardFunc(&keyDown2Static);
	glutKeyboardUpFunc(&keyUp2Static);
	glutMotionFunc(&MouseMotionStatic);
	glutPassiveMotionFunc(&MouseMotionStatic);
	glutMouseFunc(&MouseButtonStatic);

	get_error();
}

void GL_Main::deleteTexture(GLuint* tex)
{
	glDeleteTextures(1, tex);
	get_error();

	*tex = 0;
}

void GL_Main::createTexture(GLuint* tex_name, unsigned int size_x, unsigned int size_y, int bpp)
{
	// create a tex as attachment
	glGenTextures(1, tex_name);
	glBindTexture(GL_TEXTURE_2D, *tex_name);

	// set basic parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// buffer data
	if (bpp == 16)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size_x, size_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	get_error();
}

void GL_Main::reshape_static(int w, int h)
{
	screen.window_width = w;
	screen.window_height = h;
}

void GL_Main::idle_static()
{
	glutPostRedisplay();
}

void GL_Main::createPBO(GLuint* pbo, int image_width, int image_height, int bpp)
{
	// create buffer object
	glGenBuffers(1, pbo);
	glBindBuffer(GL_ARRAY_BUFFER, *pbo);

	// buffer data
	glBufferData(GL_ARRAY_BUFFER, image_width * image_height * (bpp / 8), NULL, GL_DYNAMIC_COPY);
	check_gl_err();

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// attach this Buffer Object to CUDA
	cuda_pbo_register(*pbo);

	get_error();
}

void GL_Main::deletePBO(GLuint* pbo)
{
	glBindBuffer(GL_ARRAY_BUFFER, *pbo);
	glDeleteBuffers(1, pbo);

	get_error();

	*pbo = 0;
}
