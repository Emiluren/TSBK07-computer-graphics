#version 150

in vec2 tex_coord;
out vec4 out_Color;

uniform sampler2D texUnit;

void main(void)
{
	out_Color = texture(texUnit, tex_coord);
}
