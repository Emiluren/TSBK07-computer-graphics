#version 150

in vec2 tex_coord;
in vec3 vertex_color;

out vec4 out_Color;

uniform sampler2D texUnit;

void main(void)
{
	out_Color = vec4(vertex_color, 1.0) * texture(texUnit, tex_coord);
}
