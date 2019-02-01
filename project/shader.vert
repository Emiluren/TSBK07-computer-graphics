#version 150

//in vec3 in_Color;
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
in ivec4 in_BoneIDs;
in vec4 in_Weights;

const int kMaxBones = 10;

uniform mat4 matrix;
uniform mat4 bones[kMaxBones];

out vec4 g_color;

const vec3 lightDir = normalize(vec3(0.3, 0.5, 1.0));

// Uppgift 3: Soft-skinning på GPU
//
// Flytta över din implementation av soft skinning från CPU-sidan
// till vertexshadern. Mer info finns på hemsidan.

void main(void)
{
	mat4 boneTransform =
		bones[in_BoneIDs[0]] * in_Weights[0] +
		bones[in_BoneIDs[1]] * in_Weights[1] +
		bones[in_BoneIDs[2]] * in_Weights[2] +
		bones[in_BoneIDs[3]] * in_Weights[3];

	// transformera resultatet med ModelView- och Projection-matriserna
	gl_Position = matrix * boneTransform * vec4(in_Position, 1.0);

	// sätt röd+grön färgkanal till vertex Weights
	vec4 color = vec4(in_TexCoord.x, in_TexCoord.y, 0.0, 1.0);

	// Lägg på en enkel ljussättning på vertexarna 	
	float intensity = dot(mat3(boneTransform) * in_Normal, lightDir);
	color.xyz *= intensity;

	g_color = color;
}

