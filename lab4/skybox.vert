#version 150

in vec3 inPosition;
in vec2 inTexCoord;

out vec2 tex_coord;

uniform mat4 cameraRotation;

void main(void)
{
	gl_Position = cameraRotation * vec4(inPosition, 1.0);
    tex_coord = inTexCoord;
}
