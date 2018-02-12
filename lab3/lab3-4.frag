#version 150

in vec3 vertex_normal;
in vec3 surface_position;

out vec4 out_Color;

uniform vec3 lightSourcesDirPosArr[4];
uniform vec3 lightSourcesColorArr[4];
uniform float specularExponent;
uniform bool isDirectional[4];

uniform mat4 v;

void main(void)
{
     const vec3 light = vec3(0.58, 0.58, 0.58); 
     float shade = clamp(dot(normalize(vertex_normal), mat3(v) * light), 0, 1);
     out_Color = vec4(shade, shade, shade, 1.0);
}
