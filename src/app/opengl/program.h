#pragma once

#include <app/opengl/common.h>
#include <fstream>
#include <string>

class Program
{
private:
	GLuint program_id;

	GLuint CompileShader(std::string file_path, GLenum shaderType);
public:
	Program(std::string vert_shader, std::string frag_shader);
	void UseProgram();

	GLuint FindProgramAttribute(std::string attribute) const;
};