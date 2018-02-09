#version 150

in vec3 in_Position;
in vec2 in_TexCoord;

out vec2 tex_coord;

uniform mat4 v;

void main(void)
{
	gl_Position = v * vec4(in_Position, 1.0);
    tex_coord = in_TexCoord;
}
