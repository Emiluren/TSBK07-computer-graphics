#version 150

in vec3 in_Position;
in vec3 in_Normal;
//in vec2 in_TexCoord;

//out vec2 tex_coord;
out vec3 vertex_color;

uniform mat4 mvp;
uniform mat4 mv;

void main(void)
{
	gl_Position = mvp * vec4(in_Position, 1.0);

    const vec3 light = vec3(0.58, 0.58, 0.58);
    float cosTheta = clamp(dot(mat3(mv) * in_Normal, light), 0, 1);
    vertex_color = vec3(cosTheta);

    //tex_coord = in_TexCoord;
}
