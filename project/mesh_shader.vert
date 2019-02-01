#version 150

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
in ivec4 in_BoneIDs;
in vec4 in_Weights;

const int kMaxBones = 10;

uniform mat4 matrix;
uniform mat4 bones[kMaxBones];

out float light;
out vec2 v_TexCoord;

const vec3 lightDir = normalize(vec3(0, -0.5, -0.3));

void main(void) {
	// mat4 boneTransform =
	// 	bones[in_BoneIDs[0]] * in_Weights[0] +
	// 	bones[in_BoneIDs[1]] * in_Weights[1] +
	// 	bones[in_BoneIDs[2]] * in_Weights[2] +
	// 	bones[in_BoneIDs[3]] * in_Weights[3];

	// gl_Position = matrix * boneTransform * vec4(in_Position, 1.0);
	vec3 p = vec3(in_Position.x, in_Position.z, -in_Position.y);
	gl_Position = matrix * vec4(p * 0.4, 1.0);

	float intensity = dot(in_Normal, lightDir);
	light = intensity;
	v_TexCoord = in_TexCoord;
}
