#version 150

in vec3 vertex_normal;
in vec3 world_position;

out vec4 out_Color;

uniform vec3 lightSourcesDirPosArr[4];
uniform vec3 lightSourcesColorArr[4];
uniform float specularExponent;
uniform bool isDirectional[4];

uniform vec3 worldEyePos;

void main(void)
{
    vec3 total_color = vec3(0);
    for (int i = 0; i < 4; i++) {
        vec3 lightDirection = vec3(0, 0, 0);
        if (isDirectional[i])
            lightDirection = lightSourcesDirPosArr[i];
        else
            lightDirection = normalize(lightSourcesDirPosArr[i] - world_position);
        vec3 normal = normalize(vertex_normal);
        vec3 lightColor = lightSourcesColorArr[i];

        float shade = clamp(dot(normal, lightDirection), 0, 1);

        // Specular
        vec3 reflectedLightDirection = reflect(-lightDirection, normal);
        vec3 eyeDirection = normalize(worldEyePos - world_position);
        float specularStrength = 0.0;
        if (dot(lightDirection, normal) > 0.0) {
            specularStrength = dot(reflectedLightDirection, eyeDirection);
            specularStrength = max(specularStrength, 0.01);
            specularStrength = pow(specularStrength, specularExponent);
        }
        total_color += lightColor * (shade + specularStrength);
    }
    out_Color = vec4(total_color, 1.0);
}
