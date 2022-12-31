#include "program.h"

GLuint Program::CompileShader(std::string file_path, GLenum shaderType)
{
	
	int success;
	char infoLog[512];

	std::ifstream file(file_path);
	std::string source;

	file.seekg(0, std::ios::end);
	source.reserve(file.tellg());
	file.seekg(0, std::ios::beg);

	source.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	GLuint shader = glCreateShader(shaderType);
	const GLchar* src = source.c_str();
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	GLint status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		if (shaderType == GL_VERTEX_SHADER)
			printf("Error compiling vertex shader: %s\n", infoLog);
		else
			printf("Error compiling fragment shader: %s\n", infoLog);
	}
	return shader;
}

Program::Program(std::string vert_shader, std::string frag_shader)
{
	int success;
	char infoLog[512];
	GLuint vertexShader = CompileShader(vert_shader, GL_VERTEX_SHADER);
	GLuint fragmentShader = CompileShader(frag_shader, GL_FRAGMENT_SHADER);
	program_id = glCreateProgram();
	glAttachShader(program_id, vertexShader);
	glAttachShader(program_id, fragmentShader);
	glLinkProgram(program_id);
	GLint status = GL_FALSE;
	glGetProgramiv(program_id, GL_LINK_STATUS, &status);
	if (!status)
	{
		glGetProgramInfoLog(program_id, 512, NULL, infoLog);
		printf("Error linking program: %s\n", infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Program::UseProgram()
{
	glUseProgram(program_id);
}

GLuint Program::FindProgramAttribute(std::string attribute) const
{
	const GLchar* attrib = attribute.c_str();
	GLint index = glGetAttribLocation(program_id, attrib);
	return index;
}