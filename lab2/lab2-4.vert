#version 150

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
out vec2 tex_coord;
uniform mat4 mat;

void main(void)
{
	gl_Position = mat * vec4(in_Position, 1.0);
    tex_coord = in_TexCoord;
}
