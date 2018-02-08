#version 150

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
out vec2 tex_coord;
uniform mat4 zrot;
uniform mat4 yrot;
uniform mat4 xrot;
uniform mat4 trans;

void main(void)
{
	gl_Position = trans * zrot * yrot * xrot * vec4(in_Position, 1.0);
    tex_coord = in_TexCoord;
}
