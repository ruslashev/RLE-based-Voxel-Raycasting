#pragma once

#include <math.h>
#include <vector>

#include <GL/glew.h>
#include <GL/freeglut.h>

class glShaderObject
{
	friend class glShader;

public:
	glShaderObject();
	virtual ~glShaderObject();

	int load(const char* filename);
	void loadFromMemory(const char* program);
	bool compile(void);
	const char* getCompilerLog(void);
	GLint getAttribLocation(char* attribName);

protected:
	int program_type; //!< 1 = Vertex Program, 2 = Fragment Program, 0 = none

	GLuint ShaderObject; //!< Program Object
	GLubyte* ShaderSource; //!< ASCII Source-Code

	GLcharARB* compiler_log;

	bool is_compiled; //!< true if compiled
	bool _memalloc; //!< true if shader allocated memory
};

class aVertexShader : public glShaderObject
{
public:
	aVertexShader();
	~aVertexShader();
};

class aFragmentShader : public glShaderObject
{
public:
	aFragmentShader();
	~aFragmentShader();
};

class glShader
{
public:
	glShader();
	virtual ~glShader();
	void addShader(glShaderObject* ShaderProgram); //!< add a Vertex or Fragment Program

	bool link(void); //!< Link all Shaders
	const char* getLinkerLog(void); //!< get Linker messages

	void begin(); //!< use Shader. OpenGL calls will go through shader.
	void end(); //!< Stop using this shader. OpenGL calls will go through regular pipeline.

	void setUniform1f(const char* varname, GLfloat v0);
	void setUniform2f(const char* varname, GLfloat v0, GLfloat v1);
	void setUniform3f(const char* varname, GLfloat v0, GLfloat v1, GLfloat v2);
	void setUniform4f(const char* varname, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

	void setUniform1i(const char* varname, GLint v0);
	void setUniform2i(const char* varname, GLint v0, GLint v1);
	void setUniform3i(const char* varname, GLint v0, GLint v1, GLint v2);
	void setUniform4i(const char* varname, GLint v0, GLint v1, GLint v2, GLint v3);

	void setUniform1fv(const char* varname, GLsizei count, GLfloat* value);
	void setUniform2fv(const char* varname, GLsizei count, GLfloat* value);
	void setUniform3fv(const char* varname, GLsizei count, GLfloat* value);
	void setUniform4fv(const char* varname, GLsizei count, GLfloat* value);
	void setUniform1iv(const char* varname, GLsizei count, GLint* value);
	void setUniform2iv(const char* varname, GLsizei count, GLint* value);
	void setUniform3iv(const char* varname, GLsizei count, GLint* value);
	void setUniform4iv(const char* varname, GLsizei count, GLint* value);

	void setUniformMatrix2fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value);
	void setUniformMatrix3fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value);
	void setUniformMatrix4fv(const char* varname, GLsizei count, GLboolean transpose, GLfloat* value);

	void GetUniformfv(const char* name, GLfloat* values);
	void GetUniformiv(const char* name, GLint* values);

	void setVertexAttrib1f(GLuint index, GLfloat v0);
	void setVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1);
	void setVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
	void setVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

	void manageMemory(void) { _mM = true; }

	void enable(void) { enabled = true; }
	void disable(void) { enabled = false; }

private:
	GLint GetUniLoc(const GLcharARB* name); // get location of a variable

	GLuint ProgramObject; // GLProgramObject

	GLcharARB* linker_log;
	bool is_linked;
	std::vector<glShaderObject*> ShaderList; // List of all Shader Programs

	bool _mM;
	bool enabled;
};

class glShaderManager {
public:
	glShaderManager();
	virtual ~glShaderManager();

	glShader* loadfromFile(const char* vertexFile, const char* fragmentFile);
	glShader* loadfromMemory(const char* vertexMem, const char* fragmentMem);

	bool free(glShader* o);
private:
	std::vector<glShader*> _shaderObjectList;
};
