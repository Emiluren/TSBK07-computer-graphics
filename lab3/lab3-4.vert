#version 150

in vec3 in_Position;
in vec3 in_Normal;

out vec3 vertex_normal;
out vec3 surface_position;

uniform mat4 mvp;
uniform mat4 mv;

void main(void)
{
	gl_Position = mvp * vec4(in_Position, 1.0);
    vertex_normal = mat3(mv) * in_Normal;
    surface_position = mat3(mv) * in_Position;
}
