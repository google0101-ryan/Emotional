#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <vector>

struct Vertex
{
	glm::vec3 position, color;
};

class Program;

template<class T>
class Buffer
{
private:
	GLuint vbo, vao;
	int cur_size;
	uint32_t capacity;
	Program* program;
public:
	void create(uint32_t size, Program* program);

	void AddData(std::vector<T> data);
	void Draw(GLenum mode);

	void AttachVertexData(GLuint position);
	void AttachColorData(GLuint position);
};