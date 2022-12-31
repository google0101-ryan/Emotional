#include "window.h"

#include <cstdio>
#include <cstdlib>

Window::Window()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(852, 480, "PlayStation 2, baby", NULL, NULL);
	if (!window)
	{
		printf("Error opening GLFW window\n");
		glfwTerminate();
		exit(1);
	}

	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Error initializing GLEW\n");
		glfwTerminate();
		exit(1);
	}

	glViewport(0, 0, 852, 480);
}

Window::~Window()
{
	glfwTerminate();
}

void Window::AttachRenderer(Renderer *renderer)
{
	render = renderer;
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(window);
}

void Window::Update()
{
	render->Draw();

	glfwSwapBuffers(window);
	glfwPollEvents();
}
