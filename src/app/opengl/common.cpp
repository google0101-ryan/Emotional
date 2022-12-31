#include "common.h"
#include <app/opengl/program.h>

template<class T>
void Buffer<T>::create(uint32_t size, Program* program)
{
	this->program = program;
	cur_size = 0;
	capacity = size;

	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLsizeiptr bufferSize = sizeof(T) * size;
	glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
}

template <class T>
void Buffer<T>::AddData(std::vector<T> data)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	unsigned int offset = cur_size * sizeof(T);
	unsigned int dataSize = data.size() * sizeof(T);
	glBufferSubData(GL_ARRAY_BUFFER, offset, dataSize, data.data());

	cur_size += data.size();
}

template <class T>
void Buffer<T>::Draw(GLenum mode)
{
	glBindVertexArray(vao);
	program->UseProgram();
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	glDrawArrays(mode, 0, (GLsizei)cur_size);
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	while (true)
	{
		GLenum result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED)
			break;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLsizeiptr bufferSize = sizeof(T) * capacity;
	glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
	cur_size = 0;
}

template <>
void Buffer<Vertex>::AttachVertexData(GLuint position)
{
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(position);
}

template <>
void Buffer<Vertex>::AttachColorData(GLuint position)
{
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(position);
}

template class Buffer<Vertex>;