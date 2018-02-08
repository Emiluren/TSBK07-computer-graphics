#version 150

in vec3 in_Position;
in vec3 in_Color;
out vec3 vert_Color;
uniform mat4 mat;

void main(void)
{
	gl_Position = mat * vec4(in_Position, 1.0);
    vert_Color = in_Color;
}
