#version 150

in vec3 in_Position;
uniform mat4 mat;

void main(void)
{
	gl_Position = mat * vec4(in_Position, 1.0);
}
