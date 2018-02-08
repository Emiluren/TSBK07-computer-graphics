#version 150

in vec3 in_Position;
in vec3 in_Normal;
out vec3 vert_Color;
uniform mat4 zrot;
uniform mat4 yrot;
uniform mat4 xrot;
uniform mat4 trans;

void main(void)
{
	gl_Position = trans * zrot * yrot * xrot * vec4(in_Position, 1.0);
    vert_Color = in_Normal;
}
