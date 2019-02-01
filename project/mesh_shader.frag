#version 150

in float light;
in vec2 v_TexCoord;

out vec4 fragColor;

uniform sampler2D sampler;

void main(void)
{
	fragColor = texture(sampler, v_TexCoord) * light;
}
