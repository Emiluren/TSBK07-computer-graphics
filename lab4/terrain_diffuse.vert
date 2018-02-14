#version 150

in vec3 inPosition;
in vec3 inNormal;
in vec2 inTexCoord;
out vec2 texCoord;
out vec3 color;

uniform mat4 totalMatrix;
uniform mat4 worldMatrix;

void main()
{
	texCoord = inTexCoord;
	gl_Position = totalMatrix * vec4(inPosition, 1.0);

    const vec3 light = vec3(0.58, 0.58, 0.58);
    vec3 worldNormal = (worldMatrix * vec4(inNormal, 0.0)).xyz;
    float cosTheta = clamp(dot(worldNormal, light), 0, 1);
    color = vec3(cosTheta);
}
