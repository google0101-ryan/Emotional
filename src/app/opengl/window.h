#pragma once

#include <app/opengl/common.h>
#include <app/opengl/renderer.h>

class Window
{
private:
	GLFWwindow* window;
	Renderer* render;
public:
	Window();
	~Window();

	void AttachRenderer(Renderer* renderer);

	bool ShouldClose();
	void Update();
};