#version 150

out vec4 outColor;
in vec2 texCoord;
in vec3 color;
in vec3 surfacePosition;
uniform sampler2D grassTex;
uniform sampler2D soilTex;

void main()
{
    float grassFactor = smoothstep(0.1, 0.2, surfacePosition.y);
    vec4 texColor =
        (1 - grassFactor) * texture(soilTex, texCoord) +
        grassFactor * texture(grassTex, texCoord);
	outColor = vec4(color, 1.0) * texColor;
}
