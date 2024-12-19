#include "glsl.hh"
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace std;

const char* aGLSLStrings[] = {
	"[e00] GLSL is not available!",
	"[e01] Not a valid program object!",
	"[e02] Not a valid object!",
	"[e03] Out of memory!",
	"[e04] Unknown compiler error!",
	"[e05] Linker log is not available!",
	"[e06] Compiler log is not available!",
	"[Empty]"
};

int CheckGLError(const char* file, int line)
{
	GLenum glErr, glErr2;
	int retCode = 0;

	glErr = glErr2 = glGetError();

	while (glErr != GL_NO_ERROR) {
		cout << "GL Error #" << glErr << " in File " << file << " at line: " << line << endl;
		retCode = 1;
		glErr = glGetError();
	}

	if (glErr2 != GL_NO_ERROR)
		while (1)
			usleep(100 * 1000);

	return 0;
}

glShader::glShader()
{
	ProgramObject = 0;
	linker_log = 0;
	is_linked = false;
	_mM = false;
	enabled = true;

	ProgramObject = glCreateProgram();
}

glShader::~glShader()
{
	if (linker_log != 0)
		free(linker_log);

	for (unsigned int i = 0; i < ShaderList.size(); i++) {
		glDetachShader(ProgramObject, ShaderList[i]->ShaderObject);

		// if you get an error here, you deleted the Program object first and then
		// the ShaderObject! Always delete ShaderObjects last!
		check_gl_err();

		if (_mM)
			delete ShaderList[i];
	}

	glDeleteShader(ProgramObject);
	check_gl_err();
}

void glShader::addShader(glShaderObject* ShaderProgram)
{
	if (ShaderProgram == 0)
		return;
	if (!ShaderProgram->is_compiled) {
		cout << "**warning** please compile program before adding object! trying to compile now...\n";
		if (!ShaderProgram->compile()) {
			cout << "...compile ERROR!\n";
			return;
		} else {
			cout << "...ok!\n";
		}
	}
	ShaderList.push_back(ShaderProgram);
}

bool glShader::link(void)
{
	unsigned int i;

	// already linked, detach everything first
	if (is_linked) {
		cout << "**warning** Object is already linked, trying to link again" << endl;
		for (i = 0; i < ShaderList.size(); i++) {
			glDetachShader(ProgramObject, ShaderList[i]->ShaderObject);
			check_gl_err();
		}
	}

	for (i = 0; i < ShaderList.size(); i++) {
		glAttachShader(ProgramObject, ShaderList[i]->ShaderObject);
		check_gl_err();
	}

	int linked;

	glLinkProgram(ProgramObject);
	check_gl_err();

	glGetProgramiv(ProgramObject, GL_LINK_STATUS, &linked);
	check_gl_err();

	if (linked) {
		is_linked = true;
		return true;
	} else {
		cout << "**linker error**\n";
	}

	return false;
}

const char* glShader::getLinkerLog(void)
{
	int blen = 0;
	int slen = 0;

	if (ProgramObject == 0)
		return aGLSLStrings[2];

	glGetProgramiv(ProgramObject, GL_INFO_LOG_LENGTH, &blen);
	check_gl_err();

	if (blen > 1) {
		if (linker_log != 0) {
			free(linker_log);
			linker_log = 0;
		}
		if ((linker_log = (char*)malloc(blen)) == NULL) {
			printf("ERROR: Could not allocate compiler_log buffer\n");
			return aGLSLStrings[3];
		}

		glGetProgramInfoLog(ProgramObject, blen, &slen, linker_log);
		check_gl_err();
	}

	return linker_log;
}

void glShader::begin(void)
{
	if (ProgramObject == 0)
		return;

	if (!enabled)
		return;

	if (is_linked) {
		glUseProgram(ProgramObject);
		check_gl_err();
	}
}

void glShader::end(void)
{
	if (!enabled)
		return;

	glUseProgram(0);
	check_gl_err();
}

void glShader::setUniform1f(const char* varname, GLfloat v0)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform1f(loc, v0);
}

void glShader::setUniform2f(const char* varname, GLfloat v0, GLfloat v1)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform2f(loc, v0, v1);
}

void glShader::setUniform3f(const char* varname, GLfloat v0, GLfloat v1, GLfloat v2)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform3f(loc, v0, v1, v2);
}

void glShader::setUniform4f(const char* varname, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform4f(loc, v0, v1, v2, v3);
}

void glShader::setUniform1i(const char* varname, GLint v0)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform1i(loc, v0);
}

void glShader::setUniform2i(const char* varname, GLint v0, GLint v1)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform2i(loc, v0, v1);
}

void glShader::setUniform3i(const char* varname, GLint v0, GLint v1, GLint v2)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform3i(loc, v0, v1, v2);
}

void glShader::setUniform4i(const char* varname, GLint v0, GLint v1, GLint v2, GLint v3)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform4i(loc, v0, v1, v2, v3);
}

void glShader::setUniform1fv(const char* varname, GLsizei count, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform1fv(loc, count, value);
}

void glShader::setUniform2fv(const char* varname, GLsizei count, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform2fv(loc, count, value);
}

void glShader::setUniform3fv(const char* varname, GLsizei count, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform3fv(loc, count, value);
}

void glShader::setUniform4fv(const char* varname, GLsizei count, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform4fv(loc, count, value);
}

void glShader::setUniform1iv(const char* varname, GLsizei count, GLint* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform1iv(loc, count, value);
}

void glShader::setUniform2iv(const char* varname, GLsizei count, GLint* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform2iv(loc, count, value);
}

void glShader::setUniform3iv(const char* varname, GLsizei count, GLint* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform3iv(loc, count, value);
}

void glShader::setUniform4iv(const char* varname, GLsizei count, GLint* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniform4iv(loc, count, value);
}

void glShader::setUniformMatrix2fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniformMatrix2fv(loc, count, transpose, value);
}

void glShader::setUniformMatrix3fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniformMatrix3fv(loc, count, transpose, value);
}

void glShader::setUniformMatrix4fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value)
{
	if (!enabled)
		return;

	GLint loc = GetUniLoc(varname);
	if (loc == -1)
		return;

	glUniformMatrix4fv(loc, count, transpose, value);
	check_gl_err();
}

GLint glShader::GetUniLoc(const char* name)
{
	GLint loc = glGetUniformLocation(ProgramObject, name);

	if (loc == -1)
		cout << "Error: can't find uniform variable \"" << name << "\"\n";

	check_gl_err();

	return loc;
}

void glShader::GetUniformfv(const char* name, GLfloat* values)
{
	GLint loc = glGetUniformLocation(ProgramObject, name);

	if (loc == -1)
		cout << "Error: can't find uniform variable \"" << name << "\"\n";

	glGetUniformfv(ProgramObject, loc, values);
}

void glShader::GetUniformiv(const char* name, GLint* values)
{
	GLint loc = glGetUniformLocation(ProgramObject, name);

	if (loc == -1)
		cout << "Error: can't find uniform variable \"" << name << "\"\n";

	glGetUniformiv(ProgramObject, loc, values);
}

void glShader::setVertexAttrib1f(GLuint index, GLfloat v0)
{
	if (!enabled)
		return;

	glVertexAttrib1f(index, v0);
}

void glShader::setVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1)
{
	if (!enabled)
		return;

	glVertexAttrib2f(index, v0, v1);
}

void glShader::setVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2)
{
	if (!enabled)
		return;

	glVertexAttrib3f(index, v0, v1, v2);
}

void glShader::setVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	if (!enabled)
		return;

	glVertexAttrib4f(index, v0, v1, v2, v3);
}

glShaderObject::glShaderObject()
{
	compiler_log = 0;
	is_compiled = false;
	program_type = 0;
	ShaderObject = 0;
	ShaderSource = 0;
	_memalloc = false;
}

glShaderObject::~glShaderObject()
{
	if (compiler_log != 0)
		free(compiler_log);

	if (ShaderSource != 0) {
		if (_memalloc)
			delete[] ShaderSource;
	}

	if (is_compiled) {
		glDeleteShader(ShaderObject);
		check_gl_err();
	}
}

unsigned long getFileLength(ifstream& file)
{
	if (!file.good())
		return 0;

	unsigned long pos = file.tellg();
	file.seekg(0, ios::end);
	unsigned long len = file.tellg();
	file.seekg(ios::beg);

	return len;
}

int glShaderObject::load(const char* filename)
{
	printf("Loading Shader %s...\n", filename);

	ifstream file;
	file.open(filename, ios::in);
	if (!file)
		return -1;

	unsigned long len = getFileLength(file);
	file.close();

	if (len == 0)
		return -2; // "Empty File"

	// there is already a source loaded, free it!
	if (ShaderSource != 0)
		if (_memalloc)
			delete[] ShaderSource;

	ShaderSource = (GLubyte*)new char[len + 1];
	if (ShaderSource == 0)
		return -3; // can't reserve memory

	_memalloc = true;

	// len isn't always strlen cause some characters are stripped in ascii
	// read... it is important to 0-terminate the real length later, len is
	// just max possible value...
	ShaderSource[len] = 0;

	FILE* fn = fopen(filename, "rb");
	fread(ShaderSource, len, 1, fn);
	fclose(fn);

	return 0;
}

void glShaderObject::loadFromMemory(const char* program)
{
	// there is already a source loaded, free it!
	if (ShaderSource != 0)
		if (_memalloc)
			delete[] ShaderSource;

	_memalloc = false;
	ShaderSource = (GLubyte*)program;
}

const char* glShaderObject::getCompilerLog(void)
{
	int blen = 0;
	int slen = 0;

	if (ShaderObject == 0)
		return aGLSLStrings[1]; // not a valid program object

	glGetShaderiv(ShaderObject, GL_INFO_LOG_LENGTH, &blen);
	check_gl_err();

	if (blen > 1) {
		if (compiler_log != 0) {
			free(compiler_log);
			compiler_log = 0;
		}
		if ((compiler_log = (char*)malloc(blen)) == NULL) {
			printf("ERROR: Could not allocate compiler_log buffer\n");
			return aGLSLStrings[3];
		}

		glGetShaderInfoLog(ShaderObject, blen, &slen, compiler_log);
		check_gl_err();
	}

    return compiler_log;
}

bool glShaderObject::compile(void)
{
	is_compiled = false;

	int compiled = 0;

	if (ShaderSource == 0)
		return false;

	GLint length = (GLint)strlen((const char*)ShaderSource);
	glShaderSource(ShaderObject, 1, (const char**)&ShaderSource, &length);
	check_gl_err();

	glCompileShader(ShaderObject);
	check_gl_err();
	glGetShaderiv(ShaderObject, GL_COMPILE_STATUS, &compiled);
	check_gl_err();

	if (compiled)
		is_compiled = true;

	return is_compiled;
}

GLint glShaderObject::getAttribLocation(char* attribName)
{
	return glGetAttribLocation(ShaderObject, attribName);
}

aVertexShader::aVertexShader()
{
	program_type = 1;
	ShaderObject = glCreateShader(GL_VERTEX_SHADER);
	check_gl_err();
}

aVertexShader::~aVertexShader() { }

aFragmentShader::aFragmentShader()
{
	program_type = 2;
	ShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	check_gl_err();
}

aFragmentShader::~aFragmentShader() { }

glShaderManager::glShaderManager() { }

glShaderManager::~glShaderManager()
{
	vector<glShader*>::iterator i = _shaderObjectList.begin();

	while (i != _shaderObjectList.end()) {
		i = _shaderObjectList.erase(i);
	}
}

glShader* glShaderManager::loadfromFile(const char* vertexFile, const char* fragmentFile)
{
	glShader* o = new glShader();

	aVertexShader* tVertexShader = new aVertexShader;
	aFragmentShader* tFragmentShader = new aFragmentShader;

	const char *log;

	if (vertexFile != 0)
		if (tVertexShader->load(vertexFile) != 0) {
			cout << "error: can't load vertex shader!\n";
			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if (fragmentFile != 0)
		if (tFragmentShader->load(fragmentFile) != 0) {
			cout << "error: can't load fragment shader!\n";
			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if (vertexFile != 0)
		if (!tVertexShader->compile()) {
			cout << "*** COMPILER ERROR (Vertex Shader):\n";
			cout << tVertexShader->getCompilerLog() << endl;
			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if ((log = tVertexShader->getCompilerLog()) != NULL) {
		cout << "*** GLSL Compiler Log (Vertex Shader):\n";
		cout << log << "\n";
	}

	if (fragmentFile != 0)
		if (!tFragmentShader->compile()) {
			cout << "*** COMPILER ERROR (Fragment Shader):\n";
			cout << tFragmentShader->getCompilerLog() << endl;

			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if ((log = tFragmentShader->getCompilerLog()) != NULL) {
		cout << "*** GLSL Compiler Log (Fragment Shader):\n";
		cout << log << "\n";
	}

	// Add to object
	if (vertexFile != 0)
		o->addShader(tVertexShader);

	if (fragmentFile != 0)
		o->addShader(tFragmentShader);

	if (!o->link()) {
		cout << "*** LINKER ERROR\n";
		cout << o->getLinkerLog() << endl;
		delete o;
		delete tVertexShader;
		delete tFragmentShader;
		return 0;
	}

	if ((log = o->getLinkerLog()) != NULL) {
		cout << "*** GLSL Linker Log:\n";
		cout << log << endl;
	}

	_shaderObjectList.push_back(o);
	o->manageMemory();

	return o;
}

glShader* glShaderManager::loadfromMemory(const char* vertexMem, const char* fragmentMem)
{
	glShader* o = new glShader();

	aVertexShader* tVertexShader = new aVertexShader;
	aFragmentShader* tFragmentShader = new aFragmentShader;

	const char *log;

	// get vertex program
	if (vertexMem != 0)
		tVertexShader->loadFromMemory(vertexMem);

	// get fragment program
	if (fragmentMem != 0)
		tFragmentShader->loadFromMemory(fragmentMem);

	// Compile vertex program
	if (vertexMem != 0)
		if (!tVertexShader->compile()) {
			cout << "*** COMPILER ERROR (Vertex Shader):\n";
			cout << tVertexShader->getCompilerLog() << endl;
			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if ((log = tVertexShader->getCompilerLog()) != NULL) {
		cout << "*** GLSL Compiler Log (Vertex Shader):\n";
		cout << log << "\n";
	}

	// Compile fragment program
	if (fragmentMem != 0)
		if (!tFragmentShader->compile()) {
			cout << "*** COMPILER ERROR (Fragment Shader):\n";
			cout << tFragmentShader->getCompilerLog() << endl;

			delete o;
			delete tVertexShader;
			delete tFragmentShader;
			return 0;
		}

	if ((log = tFragmentShader->getCompilerLog()) != NULL) {
		cout << "*** GLSL Compiler Log (Fragment Shader):\n";
		cout << log << "\n";
	}

	// Add to object
	if (vertexMem != 0)
		o->addShader(tVertexShader);
	if (fragmentMem != 0)
		o->addShader(tFragmentShader);

	// link
	if (!o->link()) {
		cout << "*** LINKER ERROR\n";
		cout << o->getLinkerLog() << endl;
		delete o;
		delete tVertexShader;
		delete tFragmentShader;
		return 0;
	}

	if ((log = o->getLinkerLog()) != NULL) {
		cout << "*** GLSL Linker Log:\n";
		cout << log << endl;
	}

	_shaderObjectList.push_back(o);
	o->manageMemory();

	return o;
}

bool glShaderManager::free(glShader* o)
{
	vector<glShader*>::iterator i = _shaderObjectList.begin();

	while (i != _shaderObjectList.end()) {
		if ((*i) == o) {
			_shaderObjectList.erase(i);
			delete o;
			return true;
		}
		i++;
	}

	return false;
}
