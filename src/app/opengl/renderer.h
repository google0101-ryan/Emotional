#pragma once

#include <app/opengl/common.h>
#include <app/opengl/program.h>
#include <vector>

class Renderer
{
private:
	Program* program;

	GLuint VBO;
	GLuint VAO;
public:
	Renderer();
	std::vector<Vertex> vertices;
	void Draw();
};