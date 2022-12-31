#version 330 core

in layout(location = 0) vec3 aPos;
in layout(location = 1) vec3 aColor;

out vec3 ourColor;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	ourColor = aColor;
}