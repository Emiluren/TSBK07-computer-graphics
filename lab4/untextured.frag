#version 150

out vec4 outColor;
in vec3 color;
uniform sampler2D tex;

void main()
{
	outColor = vec4(color, 1.0);
}
