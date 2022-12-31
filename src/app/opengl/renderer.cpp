#include "renderer.h"

Renderer::Renderer()
{
	program = new Program("shaders/vert.glsl", "shaders/frag.glsl");
	program->UseProgram();

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glGenVertexArrays(1, &VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
}
void Renderer::Draw()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	program->UseProgram();

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  
	glDrawArrays(GL_TRIANGLES, 0, vertices.size()*3);
	vertices.clear();
}
