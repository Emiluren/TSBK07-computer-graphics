#version 150

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;

out vec3 vertex_normal;
out vec3 world_position;
out vec2 vertex_uvs;

uniform mat4 WVP;
uniform mat4 World;

void main(void)
{
	gl_Position = WVP * vec4(in_Position, 1.0);
    vertex_normal = (World * vec4(in_Normal, 0.0)).xyz;
    world_position = (World * vec4(in_Position, 1.0)).xyz;
    vertex_uvs = in_TexCoord;
}
