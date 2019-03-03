// Part B: Many-bone worm

// New version by Ingemar 2010
// Removed all dependencies of the Wild Magic (wml) library.
// Replaced it with VectorUtils2 (in source)
// Replaced old shader module with the simpler "ShaderUtils" unit.

// 2013: Adapted to VectorUtils3 and MicroGlut.

// gcc skinning2.c ../common/*.c -lGL -o skinning2 -I../common
// not working any more. This is untested but closer to the truth:
// gcc skinning2.c -o skinning2 ../common/*.c ../common/Linux/MicroGlut.c -I../common -I../common/Linux -DGL_GLEXT_PROTOTYPES -lXt -lX11 -lGL -lm

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef __APPLE__
// Mac
#include <OpenGL/gl3.h>
//uses framework Cocoa
#else
#ifdef WIN32
// MS
#include <stdio.h>
#include <GL/glew.h>
#else
// Linux
#include <GL/gl.h>
#endif
#endif

#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <map>
#include <vector>
#include <limits>
using namespace std;

// Ref till shader
GLuint g_shader, g_mesh_shader;


// vec2 is mostly useful for texture coordinates, otherwise you don't use it much.
// That is why VectorUtils3 doesn't support it (yet)
typedef struct vec2
{
	GLfloat s, t;
}
vec2, *vec2Ptr;


typedef struct Triangle
{
	GLuint        v1;
	GLuint        v2;
	GLuint        v3;
} Triangle;

#define CYLINDER_SEGMENT_LENGTH 0.37
#define kMaxRow 100
#define kMaxCorners 8
#define kMaxBones 10
#define kMaxg_poly ((kMaxRow-1) * kMaxCorners * 2)
#ifndef Pi
#define Pi 3.1416
#endif
#ifndef true
#define true 1
#endif

#define BONE_LENGTH 4.0

const uint MAX_BONES = 100;

Triangle g_poly[kMaxg_poly];

// vertices
vec3 g_vertsOrg[kMaxRow][kMaxCorners];
vec3 g_normalsOrg[kMaxRow][kMaxCorners];
vec3 g_vertsRes[kMaxRow][kMaxCorners];
vec3 g_normalsRes[kMaxRow][kMaxCorners];

// vertex attributes
int g_boneIDs[kMaxRow][kMaxCorners][4];
float g_boneWeights[kMaxRow][kMaxCorners][4];
vec2 g_boneWeightVis[kMaxRow][kMaxCorners]; // Copy data to here to visualize your weights

mat4 boneRestMatrices[kMaxBones];

Model *cylinderModel; // Collects all the above for drawing with glDrawElements

mat4 modelViewMatrix, projectionMatrix;

struct Quaternion {
	float x, y, z, w;

	Quaternion normalize() {
		float length = sqrt(x*x + y*y + z*z + w*w);
		return Quaternion {
			x / length,
			y / length,
			z / length,
			w / length
		};
	}
};

struct VertexBoneData {
	VertexBoneData() : index{0}, weight{0} {}
	GLuint index;
	GLfloat weight;
};

const uint BONES_PER_VERTEX = 4;

struct VertexData {
	vec3 position;
	vec3 normal;
	vec2 tex_coord;
	GLuint bone_indices[BONES_PER_VERTEX];
	GLfloat bone_weights[BONES_PER_VERTEX];
};

struct Mesh {
    GLuint VAO;
    GLuint buffer;
	GLuint index_buffer;
	uint num_indices;
	uint material_index;

	vector<mat4> bone_offsets;
	map<string, uint> bone_index_mapping;
};

mat4 convert_assimp_matrix4(const aiMatrix4x4 ai_mat) {
	mat4 mat;
	mat.m[0] = ai_mat.a1; mat.m[1] = ai_mat.a2; mat.m[2] = ai_mat.a3; mat.m[3] = ai_mat.a4;
	mat.m[4] = ai_mat.b1; mat.m[5] = ai_mat.b2; mat.m[6] = ai_mat.b3; mat.m[7] = ai_mat.b4;
	mat.m[8] = ai_mat.c1; mat.m[9] = ai_mat.c2; mat.m[10] = ai_mat.c3; mat.m[11] = ai_mat.c4;
	mat.m[12] = ai_mat.d1; mat.m[13] = ai_mat.d2; mat.m[14] = ai_mat.d3; mat.m[15] = ai_mat.d4;
	return mat;
}

mat4 convert_assimp_matrix3(const aiMatrix3x3 ai_mat) {
	mat4 mat;
	mat.m[0] = ai_mat.a1; mat.m[1] = ai_mat.a2; mat.m[2] = ai_mat.a3; mat.m[3] = 0;
	mat.m[4] = ai_mat.b1; mat.m[5] = ai_mat.b2; mat.m[6] = ai_mat.b3; mat.m[7] = 0;
	mat.m[8] = ai_mat.c1; mat.m[9] = ai_mat.c2; mat.m[10] = ai_mat.c3; mat.m[11] = 0;
	mat.m[12] = 0;        mat.m[13] = 0;        mat.m[14] = 0;        mat.m[15] = 1;
	return mat;
}

inline Quaternion expand_quat(vec3 q) {
	float t = 1.0f - (q.x * q.x) - (q.y * q.y) - (q.z * q.z);
	float w = t < 0 ? 0 : sqrtf(t);

	return Quaternion { q.x, q.y, q.z, w };
}

mat4 quat_to_mat4(Quaternion q) {
	mat4 rot {
		1 - 2*q.y*q.y - 2*q.z*q.z, 2*q.x*q.y - 2*q.z*q.w,   2*q.x*q.z + 2*q.y*q.w,
		2*q.x*q.y + 2*q.z*q.w,     1 - q.x*q.x - 2*q.z*q.z, 2*q.y*q.z - 2*q.x*q.w,
		2*q.x*q.z - 2*q.y*q.w,     2*q.y*q.z + 2*q.x*q.w,   1 - 2*q.x*q.x - 2*q.y*q.y
	};
	return rot;
}

vec3 rotate_vec_by_quat(vec3 vec, Quaternion quat) {
	vec3 q_xyz = vec3 { quat.x, quat.y, quat.z };
	float w = quat.w;

	return
		2.0f * DotProduct(q_xyz, vec) * q_xyz +
		(w*w - DotProduct(q_xyz, q_xyz)) * vec +
		2.0f * w * CrossProduct(q_xyz, vec);
}

Quaternion operator*(Quaternion q1, Quaternion q2) {
	Quaternion q3;
	q3.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
	q3.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
	q3.y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
	q3.z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;

	return q3;
}

Quaternion operator*(float f, Quaternion q) {
	return Quaternion {
		q.x*f, q.y*f, q.z*f, q.w*f
	};
}

Quaternion operator/(Quaternion q, float f) {
	return Quaternion {
		q.x/f, q.y/f, q.z/f, q.w/f
	};
}

Quaternion operator+(Quaternion q1, Quaternion q2) {
	return Quaternion {
		q1.x + q2.x,
		q1.y + q2.y,
		q1.z + q2.z,
		q1.w + q2.w
	};
}

float mix(float x, float y, float factor) {
	return factor*x + (1.0f - factor)*y;
}

Quaternion mix(Quaternion q1, Quaternion q2, float factor) {
	float const cosTheta = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;

	if(cosTheta > 1.0f - numeric_limits<float>::epsilon()) {
		return Quaternion {
			mix(q1.w, q2.w, factor),
			mix(q1.x, q2.x, factor),
			mix(q1.y, q2.y, factor),
			mix(q1.z, q2.z, factor)
		};
	} else {
		float angle = acos(cosTheta);
		return (sin((1.0f - factor) * angle) * q1 +
				sin(factor * angle) * q2) /
			sin(angle);
	}
}

struct Md5Joint {
	int parent_index;
	vec3 position;
	Quaternion orientation;
};

struct Md5Vertex {
	Md5Vertex() :
		pos{0, 0, 0}, normal{0, 0, 0}, tex_coords{0, 0},
		start_weight{0}, weight_count{0} {}

	vec3 pos;
	vec3 normal;
	vec2 tex_coords;
	int start_weight;
	int weight_count;
};

struct Md5Weight {
	int joint_id;
	float bias;
	vec3 position;
};

struct Md5Mesh {
	string texture_filename;
	vector<Md5Vertex> vertices;
	vector<array<int, 3>> triangles;
	vector<Md5Weight> weights;
};

struct Md5Animation {
	struct JointInfo {
		int parent_id;
		int flags;
		int start_index;
	};

	struct Bound {
		vec3 min;
		vec3 max;
	};

	struct BaseFrame {
		vec3 pos;
		Quaternion orientation;
	};

	struct FrameData {
		int frame_id;
		vector<float> frame_data;
	};

	struct SkeletonJoint {
		int parent;
		vec3 pos;
		Quaternion orient;
	};

	struct FrameSkeleton {
		vector<SkeletonJoint> joints;
	};

	vector<JointInfo> joint_infos;
	vector<Bound> bounds;
	vector<BaseFrame> base_frames;
	vector<FrameData> frames;
	vector<FrameSkeleton> skeletons;
	FrameSkeleton animated_skeleton;

	int num_frames;
	int num_joints;
	int frame_rate;
	int num_animated_components;

	float anim_duration;
	float frame_duration;
	float anim_time;

	void update(float delta_time);
	void interpolate_skeletons(
		const vector<SkeletonJoint>& skeleton0,
		const vector<SkeletonJoint>& skeleton1,
		float factor
	);
};

struct Md5Model {
	vector<Md5Mesh> meshes;
	vector<Md5Joint> joints;
	vector<mat4> bind_pose;
	vector<mat4> inverse_bind_pose;
};

void compute_vertex_positions(
	const vector<Md5Joint>& joints,
	const vector<Md5Weight>& weights,
	vector<Md5Vertex>& vertices
) {
	for (auto& vert : vertices) {
		for (int i = 0; i < vert.weight_count; i++) {
			Md5Weight weight = weights[vert.start_weight + i];
			Md5Joint joint = joints[weight.joint_id];

			vec3 rot_pos = rotate_vec_by_quat(joint.position, joint.orientation);
			vert.pos += (joint.position + rot_pos) * weight.bias;
		}
	}
}

void compute_normals(
	const vector<array<int, 3>>& triangles,
	vector<Md5Vertex>& vertices
) {
	for (auto& tri : triangles) {
		vec3 v0 = vertices[tri[0]].pos;
		vec3 v1 = vertices[tri[1]].pos;
		vec3 v2 = vertices[tri[2]].pos;

		vec3 normal = CrossProduct(v2 - v0, v1 - v0);
		vertices[tri[0]].normal += normal;
		vertices[tri[1]].normal += normal;
		vertices[tri[2]].normal += normal;
	}

	for (auto& vert : vertices) {
		vert.normal = Normalize(vert.normal);
	}
}

void transform_normals_to_joint_local_space(
	const vector<Md5Joint>& joints,
	const vector<Md5Weight>& weights,
	vector<Md5Vertex>& vertices
) {
	for (auto& vert : vertices) {
		vec3 normal = vert.normal;
		vert.normal = vec3 { 0, 0, 0 };

		for (int i = 0; i < vert.weight_count; i++) {
			Md5Weight weight = weights[vert.start_weight + i];
			Md5Joint joint = joints[weight.joint_id];
			vert.normal += rotate_vec_by_quat(normal, joint.orientation) * weight.bias;
		}
	}
}

Md5Model parse_md5_mesh(const char* filename) {
	Md5Model model;

	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "parse_md5_mesh: Could not open %s\n", filename);
		return model;
	}

	// TODO: This function does not check for parse errors

	int version;
	fscanf(file, "MD5Version %d ", &version);
	assert(version == 10);

	fscanf(file, "commandline %*s ");

	int num_joints, num_meshes;
	fscanf(file, "numJoints %d ", &num_joints);
	fscanf(file, "numMeshes %d ", &num_meshes);

	// printf("joints: %d, meshes %d\n", num_joints, num_meshes);

	model.joints.reserve(num_joints);
	fscanf(file, "joints { ");
	for (int i = 0; i < num_joints; i++) {
		// char joint_name[32]; // This probably risks a buffer overflow
		// int parent_index;
		// vec3 position;
		// vec3 orientation;
		Md5Joint joint;
		vec3 orientation;

		fscanf(
			file,
			"%*s %d ( %f %f %f ) ( %f %f %f ) %*[^\n] ",
			//joint_name,
			&joint.parent_index,
			&joint.position.x, &joint.position.y, &joint.position.z,
			&orientation.x, &orientation.y, &orientation.z
		);
		joint.orientation = expand_quat(orientation);

		model.joints.push_back(joint);

		// printf(
		// 	"parsed %s: %d (%f %f %f) (%f %f %f)\n",
		// 	joint_name,
		// 	parent_index,
		// 	position.x, position.y, position.z,
		// 	orientation.x, orientation.y, orientation.z
		// );
	}
	fscanf(file, "} ");

	model.bind_pose.reserve(num_joints);
	model.inverse_bind_pose.reserve(num_joints);
	for (const auto& joint : model.joints) {
		mat4 translation = T(joint.position.x, joint.position.y, joint.position.z);
		mat4 rot = quat_to_mat4(joint.orientation);

		mat4 bone_matrix = Mult(translation, rot);
		model.bind_pose.push_back(bone_matrix);
		model.inverse_bind_pose.push_back(InvertMat4(bone_matrix));
	}

	model.meshes.resize(num_meshes);
	for (auto& mesh : model.meshes) {
		fscanf(file, "mesh { ");

		char texture_filename[128];
		fscanf(file, "shader %s ", texture_filename);
		mesh.texture_filename = string(texture_filename); // TODO: remove quotes

		int num_verts;
		fscanf(file, "numverts %d ", &num_verts);
		mesh.vertices.resize(num_verts);
		for (auto& vert : mesh.vertices) {
			fscanf(
				file,
				"vert %*d ( %f %f ) %d %d ",
				&vert.tex_coords.s, &vert.tex_coords.t,
				&vert.start_weight, &vert.weight_count
			);

			// printf(
			// 	"parsed vertex %d (%f %f) w %d %d\n",
			// 	vert_index,
			// 	tex_coords.s, tex_coords.t,
			// 	start_weight, weight_count
			// );
		}

		int num_triangles;
		fscanf(file, "numtris %d ", &num_triangles);
		mesh.triangles.resize(num_triangles);
		for (auto& tri : mesh.triangles) {
			fscanf(
				file,
				"tri %*d %d %d %d ",
				&tri[0], &tri[1], &tri[2]
			);

			// printf(
			// 	"parsed triangle %d (%d %d %d)\n", triangle_index,
			// 	vert_indices[0], vert_indices[1], vert_indices[2]
			// );
		}

		int num_weights;
		fscanf(file, "numweights %d ", &num_weights);
		mesh.weights.resize(num_weights);
		for (auto& w : mesh.weights) {
			fscanf(
				file,
				"weight %*d %d %f ( %f %f %f ) ",
				&w.joint_id, &w.bias,
				&w.position.x, &w.position.y, &w.position.z
			);

			// printf(
			// 	"parsed weight %d %d %f (%f %f %f)\n",
			// 	weight_index, joint_index, weight_bias,
			// 	weight_position.x, weight_position.y, weight_position.z
			// );
		}

		fscanf(file, "} ");
	}
	fclose(file);

	for (auto& mesh : model.meshes) {
		compute_vertex_positions(model.joints, mesh.weights, mesh.vertices);
	}

	return model;
}

const int POS_X = 1;
const int POS_Y = 2;
const int POS_Z = 4;
const int ORIENT_X = 8;
const int ORIENT_Y = 16;
const int ORIENT_Z = 32;

Md5Animation::FrameSkeleton build_frame_skeleton(
	const vector<Md5Animation::JointInfo>& joint_infos,
	const vector<Md5Animation::BaseFrame>& base_frames,
	const Md5Animation::FrameData& frame
) {
	Md5Animation::FrameSkeleton skeleton;

	for (uint i = 0; i < joint_infos.size(); i++) {
		unsigned int j = 0;
 
        const Md5Animation::JointInfo& joint_info = joint_infos[i];
        // Start with the base frame position and orientation.
		Md5Animation::SkeletonJoint animated_joint;
		animated_joint.parent = joint_info.parent_id;
		animated_joint.pos = base_frames[i].pos;
		animated_joint.orient = base_frames[i].orientation;
        animated_joint.parent = joint_info.parent_id;
 
        if (joint_info.flags & POS_X) {
            animated_joint.pos.x = frame.frame_data[ joint_info.start_index + j++ ];
        }
        if (joint_info.flags & POS_Y) {
            animated_joint.pos.y = frame.frame_data[ joint_info.start_index + j++ ];
        }
        if (joint_info.flags & POS_Z) {
            animated_joint.pos.z  = frame.frame_data[ joint_info.start_index + j++ ];
        }
        if (joint_info.flags & ORIENT_X) {
            animated_joint.orient.x = frame.frame_data[ joint_info.start_index + j++ ];
        }
        if (joint_info.flags & ORIENT_Y) {
            animated_joint.orient.y = frame.frame_data[ joint_info.start_index + j++ ];
        }
        if (joint_info.flags & ORIENT_Z) {
            animated_joint.orient.z = frame.frame_data[ joint_info.start_index + j++ ];
        }

		animated_joint.orient = expand_quat(
			vec3 {
				animated_joint.orient.x, animated_joint.orient.y, animated_joint.orient.z
			}
		);

		if (animated_joint.parent >= 0) {
			Md5Animation::SkeletonJoint parent_joint = skeleton.joints[animated_joint.parent];
			vec3 rot_pos = rotate_vec_by_quat(animated_joint.pos, parent_joint.orient);

			animated_joint.pos = parent_joint.pos + rot_pos;
			animated_joint.orient = (parent_joint.orient * animated_joint.orient).normalize();
		}

		skeleton.joints.push_back(animated_joint);
	}

	return skeleton;
}

Md5Animation parse_md5_anim(const char* filename) {
	Md5Animation animation;
	FILE* file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "parse_md5_mesh: Could not open %s\n", filename);
		return animation;
	}

	// TODO: This function does not check for parse errors

	int version;
	fscanf(file, "MD5Version %d ", &version);
	assert(version == 10);

	fscanf(file, "commandline %*s ");

	fscanf(file, "numFrames %d ", &animation.num_frames);
	fscanf(file, "numJoints %d ", &animation.num_joints);
	fscanf(file, "frameRate %d ", &animation.frame_rate);
	fscanf(file, "numAnimatedCompontents %d ", &animation.num_animated_components);

	animation.frame_duration = 1.0f / (float)animation.frame_rate;
	animation.anim_duration = animation.frame_duration * (float)animation.num_frames;
	animation.anim_time = 0.0f;

	animation.joint_infos.resize(animation.num_joints);
	fscanf(file, "hierarchy { ");
	for (auto& joint_info : animation.joint_infos) {
		fscanf(
			file,
			"%*s %d %d %d %*[^\n] ",
			&joint_info.parent_id,
			&joint_info.flags,
			&joint_info.start_index
		);
	}
	fscanf(file, "} ");

	animation.bounds.resize(animation.num_frames);
	fscanf(file, "bounds { ");
	for (auto& bound : animation.bounds) {
		fscanf(
			file,
			"( %f %f %f ) ( %f %f %f ) ",
			&bound.min.x, &bound.min.y, &bound.min.z,
			&bound.max.x, &bound.max.y, &bound.max.z
		);
	}
	fscanf(file, "} ");

	animation.base_frames.resize(animation.num_joints);
	fscanf(file, "baseframe { ");
	for (auto& base_frame : animation.base_frames) {
		vec3 orientation;
		fscanf(
			file,
			"( %f %f %f ) ( %f %f %f ) ",
			&base_frame.pos.x, &base_frame.pos.y, &base_frame.pos.z,
			&orientation.x, &orientation.y, &orientation.z
		);
		base_frame.orientation = expand_quat(orientation);
	}
	fscanf(file, "} ");

	animation.frames.resize(animation.num_frames);
	for (auto& frame : animation.frames) {
		fscanf(file, "frame { ");

		frame.frame_data.resize(animation.num_animated_components);
		for (auto& data : frame.frame_data) {
			fscanf(file, "%f ", &data);
		}
		fscanf(file, "} ");

		animation.skeletons.push_back(
			build_frame_skeleton(
				animation.joint_infos,
				animation.base_frames,
				frame
			)
		);
	}

	return animation;
}

void Md5Animation::update(float delta_time) {
	if (num_frames < 1) {
		return;
	}

	anim_time += delta_time;

	while (anim_time > anim_duration) {
		anim_time -= anim_duration;
	}
	while (anim_time < 0.0f) {
		anim_time += anim_duration;
	}
}

void Md5Animation::interpolate_skeletons(
	const vector<SkeletonJoint> &skeleton0,
	const vector<SkeletonJoint> &skeleton1,
	float factor
) {
	for (int i = 0; i < num_joints; i++) {
		SkeletonJoint& joint = animated_skeleton.joints[i];
		joint.parent = skeleton0[i].parent;
		joint.pos = skeleton0[i].pos*factor + skeleton1[i].pos*(1 - factor);
		joint.orient = mix(skeleton0[i].orient, skeleton1[2].orient, factor);
	}
}

struct Mesh load_mesh(const aiMesh* assimp_mesh) {
	Mesh mesh;

	vector<VertexData> vertices;
	vector<GLuint> indices;

	// Store buffer offsets for all vertex data
	mesh.material_index = assimp_mesh->mMaterialIndex;
	mesh.num_indices = assimp_mesh->mNumFaces * 3;

	vertices.reserve(assimp_mesh->mNumVertices);
	// Initialize vertex data
	for (uint i = 0; i < assimp_mesh->mNumVertices; i++) {
		aiVector3D pos = assimp_mesh->mVertices[i];
		aiVector3D norm = assimp_mesh->mNormals[i];
		aiVector3D tex_coord = assimp_mesh->mTextureCoords[0][i];

		VertexData vert;
		vert.position = SetVector(pos.x, pos.y, pos.z);
		vert.normal = SetVector(norm.x, norm.y, norm.z);
		vert.tex_coord = vec2 { tex_coord.x, tex_coord.y };
		for (uint i = 0; i < BONES_PER_VERTEX; i++) {
			vert.bone_indices[i] = 0;
			vert.bone_weights[i] = 0;
		}

		vertices.push_back(vert);
	}

	indices.reserve(assimp_mesh->mNumFaces * 3);
	for (uint i = 0; i < assimp_mesh->mNumFaces; i++) {
		aiFace face = assimp_mesh->mFaces[i];
		assert(face.mNumIndices == 3);
		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}

	// Initialize bones
	for (uint i = 0; i < assimp_mesh->mNumBones; i++) {
		struct aiBone* bone = assimp_mesh->mBones[i];

		// The data should be stored in the same way
		mat4 bone_offset = convert_assimp_matrix4(bone->mOffsetMatrix);
		mesh.bone_offsets.push_back(bone_offset);

		//printf("bone %s\n", bone->mName.data);

		mesh.bone_index_mapping[bone->mName.data] = i;

		for (uint k = 0; k < bone->mNumWeights; k++) {
			uint vert_id = bone->mWeights[k].mVertexId;
			float weight = bone->mWeights[k].mWeight;

			// Find an available bone
			for (uint j = 0; j < BONES_PER_VERTEX; j++) {
				if (vertices[vert_id].bone_weights[j] == 0.0f) {
					vertices[vert_id].bone_weights[j] = weight;
					vertices[vert_id].bone_indices[j] = i;
				}
			}
		}
	}

	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);

	glGenBuffers(1, &mesh.buffer);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &mesh.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	GLuint pos_loc = glGetAttribLocation(g_mesh_shader, "in_Position");
	GLuint norm_loc = glGetAttribLocation(g_mesh_shader, "in_Normal");
	GLuint texcoord_loc = glGetAttribLocation(g_mesh_shader, "in_TexCoord");
	GLuint boneid_loc = glGetAttribLocation(g_mesh_shader, "in_BoneIDs");
	GLuint weight_loc = glGetAttribLocation(g_mesh_shader, "in_Weights");

	glEnableVertexAttribArray(pos_loc);
	glEnableVertexAttribArray(norm_loc);
	glEnableVertexAttribArray(texcoord_loc);
	glEnableVertexAttribArray(boneid_loc);
	glEnableVertexAttribArray(weight_loc);

	glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);
	glVertexAttribPointer(norm_loc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
	glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, tex_coord));
	glVertexAttribIPointer(boneid_loc, BONES_PER_VERTEX, GL_INT, sizeof(VertexData), (void*)offsetof(VertexData, bone_indices));
	glVertexAttribPointer(weight_loc, BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, bone_weights));

	glBindVertexArray(0);

	return mesh;
}

///////////////////////////////////////////////////
//		I N I T  B O N E  W E I G H T S
// Desc:  initierar benvikterna
//
void initBoneWeights(void) {
	long row, corner;
	int bone;

	// sätter värden till alla vertexar i meshen
	for (row = 0; row < kMaxRow; row++) {
		for (corner = 0; corner < kMaxCorners; corner++) {
			float boneWeights[kMaxBones];
			float totalBoneWeight = 0.f;

			float maxBoneWeight = 0.f;

			for (bone = 0; bone < kMaxBones; bone++) {
				float bonePos = BONE_LENGTH * bone;
				float boneDist = fabs(bonePos - g_vertsOrg[row][corner].x);
				float boneWeight = (BONE_LENGTH - boneDist) / (BONE_LENGTH);
				if (boneWeight < 0)
					boneWeight = 0;
				boneWeights[bone] = boneWeight;
				totalBoneWeight += boneWeight;

				if (maxBoneWeight < boneWeight)
					maxBoneWeight = boneWeight;
			}

			// Set all ids and weights to 0 so unused data doesn't destroy anything
			for (int bone_id = 0; bone_id < 4; bone_id++) {
				g_boneIDs[row][corner][bone_id] = 0;
				g_boneWeights[row][corner][bone_id] = 0;
			}

			int bone_i = 0;
			g_boneWeightVis[row][corner].s = 0;
			g_boneWeightVis[row][corner].t = 0;
			//printf("for vert %d %d: ", row, corner);
			for (bone = 0; bone < kMaxBones; bone++) {
				// Use the first 4 bones that have any weight for this vertex
				float weight = boneWeights[bone] / totalBoneWeight;
				if (weight > 0 && bone_i < 4) {
					g_boneIDs[row][corner][bone_i] = bone;
					g_boneWeights[row][corner][bone_i] = weight;
					//printf("b %d, w %f ", bone, weight);
					bone_i++;
				}
				if (bone & 1) g_boneWeightVis[row][corner].s += weight; // Copy data to here to visualize your weights or anything else
				if ((bone+1) & 1) g_boneWeightVis[row][corner].t += weight; // Copy data to here to visualize your weights
			}
			//printf("\n");
		}
	}

	// Build Model from cylinder data
	cylinderModel = LoadDataToModel(
		(GLfloat*) g_vertsRes,
		(GLfloat*) g_normalsRes,
		(GLfloat*) g_boneWeightVis, // texCoords
		NULL, // (GLfloat*) g_boneWeights, // colors
		(GLuint*) g_poly, // indices
		kMaxRow*kMaxCorners,
		kMaxg_poly * 3);

	glBindVertexArray(cylinderModel->vao);

	// Buffer bone IDs
	GLuint boneIdBuffer;
	glGenBuffers(1, &boneIdBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, boneIdBuffer);
	glBufferData(GL_ARRAY_BUFFER, kMaxRow*kMaxCorners*sizeof(GLint)*4, g_boneIDs, GL_STATIC_DRAW);
	GLint loc = glGetAttribLocation(g_shader, "in_BoneIDs");
	if (loc >= 0) {
		glVertexAttribIPointer(loc, 4, GL_INT, 0, 0);
		glEnableVertexAttribArray(loc);
	}
	else
		fprintf(stderr, "%s warning: '%s' not found in shader!\n", "Bone ids", "in_BoneIDs");

	// Buffer bone weights
	GLuint boneWeightBuffer;
	glGenBuffers(1, &boneWeightBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, boneWeightBuffer);
	glBufferData(GL_ARRAY_BUFFER, kMaxRow*kMaxCorners*sizeof(GLfloat)*4, g_boneWeights, GL_STATIC_DRAW);

	loc = glGetAttribLocation(g_shader, "in_Weights");
	if (loc >= 0) {
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(loc);
	}
	else
		fprintf(stderr, "%s warning: '%s' not found in shader!\n", "Bone weights", "in_Weights");
}



///////////////////////////////////////////////////
//		B U I L D  C Y L I N D E R
// Desc:  bygger upp cylindern
//
void BuildCylinder()
{
	long	row, corner, cornerIndex;

	// sätter värden till alla vertexar i meshen
	for (row = 0; row < kMaxRow; row++) {
		for (corner = 0; corner < kMaxCorners; corner++) {
			g_vertsOrg[row][corner].x = (float) row * CYLINDER_SEGMENT_LENGTH;
			g_vertsOrg[row][corner].y = cos(corner * 2*Pi / kMaxCorners);
			g_vertsOrg[row][corner].z = sin(corner * 2*Pi / kMaxCorners);

			g_normalsOrg[row][corner].x = 0;
			g_normalsOrg[row][corner].y = cos(corner * 2*Pi / kMaxCorners);
			g_normalsOrg[row][corner].z = sin(corner * 2*Pi / kMaxCorners);
		}
	}

	// g_poly definerar mellan vilka vertexar som
	// trianglarna ska ritas
	for (row = 0; row < kMaxRow-1; row++) {
		for (corner = 0; corner < kMaxCorners; corner++) {
			// Quads built from two triangles

			if (corner < kMaxCorners-1) {
				cornerIndex = row * kMaxCorners + corner;
				g_poly[cornerIndex * 2].v1 = cornerIndex;
				g_poly[cornerIndex * 2].v2 = cornerIndex + 1;
				g_poly[cornerIndex * 2].v3 = cornerIndex + kMaxCorners + 1;

				g_poly[cornerIndex * 2 + 1].v1 = cornerIndex;
				g_poly[cornerIndex * 2 + 1].v2 = cornerIndex + kMaxCorners + 1;
				g_poly[cornerIndex * 2 + 1].v3 = cornerIndex + kMaxCorners;
			} else {
				// Specialfall: sista i varvet, gå runt hörnet korrekt
				cornerIndex = row * kMaxCorners + corner;
				g_poly[cornerIndex * 2].v1 = cornerIndex;
				g_poly[cornerIndex * 2].v2 = cornerIndex + 1 - kMaxCorners;
				g_poly[cornerIndex * 2].v3 = cornerIndex + kMaxCorners + 1 - kMaxCorners;

				g_poly[cornerIndex * 2 + 1].v1 = cornerIndex;
				g_poly[cornerIndex * 2 + 1].v2 = cornerIndex + kMaxCorners + 1 - kMaxCorners;
				g_poly[cornerIndex * 2 + 1].v3 = cornerIndex + kMaxCorners;
			}
		}
	}

	// lägger en kopia av orginal modellen i g_vertsRes
	memcpy(g_vertsRes,  g_vertsOrg, kMaxRow * kMaxCorners* sizeof(vec3));
	memcpy(g_normalsRes,  g_normalsOrg, kMaxRow * kMaxCorners* sizeof(vec3));
}


//////////////////////////////////////
//		B O N E
// Desc:	en enkel ben-struct med en
//			pos-vektor och en rot-vektor
//			rot vektorn skulle lika gärna
//			kunna vara av 3x3 men VectorUtils2 har bara 4x4
typedef struct Bone {
	vec3 pos;
	mat4 rot;
} Bone;


///////////////////////////////////////
//		G _ B O N E S
// vårt skelett
Bone g_bones[kMaxBones]; // Ursprungsdata, ändra ej
Bone g_bonesRes[kMaxBones]; // Animerat

mat4 boneTranslation(int b) {
	if (b == 0) {
		return T(0, 0, 0);
	} else {
		return T(BONE_LENGTH, 0, 0);
	}
}

mat4 TByVec(vec3 pos) {
	return T(pos.x, pos.y, pos.z);
}


///////////////////////////////////////////////////////
//		S E T U P  B O N E S
//
void setupBones(void)
{
	int bone;

	for (bone = 0; bone < kMaxBones; bone++) {
		g_bones[bone].pos = SetVector((float) bone * BONE_LENGTH, 0.0f, 0.0f);
		g_bones[bone].rot = IdentityMatrix();
		
		boneRestMatrices[bone] = T(-bone * BONE_LENGTH, 0.0f, 0.0f);
	}
}

///////////////////////////////////////////////////////
//		D E F O R M  C Y L I N D E R
//
// Desc:	deformera cylinder-meshen enligt skelettet
void DeformCylinder() {
	mat4 boneLocalAnimMatrices[kMaxBones];
	for (int b = 0; b < kMaxBones; ++b) {
		mat4 translation = boneTranslation(b);
		boneLocalAnimMatrices[b] = Mult(translation, g_bonesRes[b].rot);
	}

	mat4 boneAnimMatrices[kMaxBones];
	boneAnimMatrices[0] = boneLocalAnimMatrices[0];
	for(int b = 1; b < kMaxBones; ++b) {
		boneAnimMatrices[b] = Mult(
			boneAnimMatrices[b-1],
			boneLocalAnimMatrices[b]
		);
	}

	mat4 completeMatrix[kMaxBones];
	for (int b = 0; b < kMaxBones; ++b) {
		completeMatrix[b] = Mult(boneAnimMatrices[b], boneRestMatrices[b]);
	}

	GLint loc = glGetUniformLocation(g_shader, "bones");
	if (loc >= 0) {
		glUniformMatrix4fv(loc, kMaxBones, GL_TRUE, (const GLfloat*)completeMatrix);
	}
	else
		fprintf(stderr, "%s warning: '%s' not found in shader!\n", "Bone weights", "in_Weights");
}


/////////////////////////////////////////////
//		A N I M A T E  B O N E S
// Desc:	en väldigt enkel animation av skelettet
//			vrider ben 1 i en sin(counter)
void animateBones(void)
{
	int bone;
	// Hur mycket kring varje led? ändra gärna.
	float angleScales[10] = { 1, 1, 1, 1, 1, -1, -1, -1, -1, -1 };

	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	// Hur mycket skall vi vrida?
	float angle = sin(time * 3.f) / 2.0f;

	memcpy(&g_bonesRes, &g_bones, kMaxBones*sizeof(Bone));

	g_bonesRes[0].rot = Rz(angle * angleScales[0]);

	for (bone = 1; bone < kMaxBones; bone++)
	g_bonesRes[bone].rot = Rz(angle * angleScales[bone]);
}

///////////////////////////////////////////////
//		 D R A W  C Y L I N D E R
// Desc:	sätter bone-positionen i vertex shadern
void DrawCylinder()
{
	animateBones();
	DeformCylinder();
	DrawModel(cylinderModel, g_shader, "in_Position", "in_Normal", "in_TexCoord");
}

struct AnimationData {
	float animation_time;
	aiAnimation* animation;
	Mesh* mesh;
};

uint find_position(float animation_time, const aiNodeAnim* node_anim) {
    for (uint i = 0 ; i < node_anim->mNumPositionKeys - 1 ; i++) {
        if (animation_time < (float)node_anim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(false);
    return 0;
}

uint find_rotation(float animation_time, const aiNodeAnim* node_anim) {
    assert(node_anim->mNumRotationKeys > 0);

    for (uint i = 0 ; i < node_anim->mNumRotationKeys - 1 ; i++) {
        if (animation_time < (float)node_anim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(false);
    return 0;
}

uint find_scaling(float animation_time, const aiNodeAnim* node_anim) {
    assert(node_anim->mNumScalingKeys > 0);

    for (uint i = 0 ; i < node_anim->mNumScalingKeys - 1 ; i++) {
        if (animation_time < (float)node_anim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(false);
    return 0;
}

vec3 calc_interpolated_position(float animation_time, const aiNodeAnim* node_anim) {
    if (node_anim->mNumPositionKeys == 1) {
        aiVector3D pos = node_anim->mPositionKeys[0].mValue;
		return SetVector(pos.x, pos.y, pos.z);
    }

    uint position_index = find_position(animation_time, node_anim);
    uint next_position_index = position_index + 1;

    assert(next_position_index < node_anim->mNumPositionKeys);

	float delta_time =
		node_anim->mPositionKeys[next_position_index].mTime -
		node_anim->mPositionKeys[position_index].mTime;
    float factor =
		(animation_time - node_anim->mPositionKeys[position_index].mTime) / delta_time;

	assert(factor >= 0.0f && factor <= 1.0f);

	aiVector3D start = node_anim->mPositionKeys[position_index].mValue;
    aiVector3D end = node_anim->mPositionKeys[next_position_index].mValue;
    aiVector3D out = start + factor * (end - start);
	return SetVector(out.x, out.y, out.z);
}

aiQuaternion calc_interpolated_rotation(float animation_time, const aiNodeAnim* node_anim) {
    if (node_anim->mNumRotationKeys == 1) {
        return node_anim->mRotationKeys[0].mValue;
    }

    uint index = find_rotation(animation_time, node_anim);
    uint next_index = index + 1;

    assert(next_index < node_anim->mNumRotationKeys);

	float delta_time =
		node_anim->mRotationKeys[next_index].mTime -
		node_anim->mRotationKeys[index].mTime;
    float factor =
		(animation_time - node_anim->mRotationKeys[index].mTime) / delta_time;

	assert(factor >= 0.0f && factor <= 1.0f);

	aiQuaternion start = node_anim->mRotationKeys[index].mValue;
    aiQuaternion end = node_anim->mRotationKeys[next_index].mValue;

	aiQuaternion out;
    aiQuaternion::Interpolate(out, start, end, factor);
    return out;
}

vec3 calc_interpolated_scaling(float animation_time, const aiNodeAnim* node_anim) {
    if (node_anim->mNumScalingKeys == 1) {
        aiVector3D scale = node_anim->mScalingKeys[0].mValue;
        return SetVector(scale.x, scale.y, scale.z);
    }

    uint index = find_scaling(animation_time, node_anim);
    uint next_index = index + 1;

    assert(next_index < node_anim->mNumScalingKeys);

	float delta_time =
		node_anim->mScalingKeys[next_index].mTime -
		node_anim->mScalingKeys[index].mTime;
    float factor =
		(animation_time - node_anim->mScalingKeys[index].mTime) / delta_time;

	assert(factor >= 0.0f && factor <= 1.0f);

	aiVector3D start = node_anim->mScalingKeys[index].mValue;
    aiVector3D end = node_anim->mScalingKeys[next_index].mValue;
	aiVector3D out = start + factor * (end - start);
	return SetVector(out.x, out.y, out.z);
}

const aiNodeAnim* find_node_anim(const aiAnimation* animation, const string node_name) {
	for (uint i = 0; i < animation->mNumChannels; i++) {
		const aiNodeAnim* node_anim = animation->mChannels[i];
		if (node_name == node_anim->mNodeName.data) {
			return node_anim;
		}
	}
	return nullptr;
}

void read_node_heirarchy(
	const AnimationData anim_data,
	const aiNode* node,
	const mat4 parent_transform,
	mat4 bone_transforms[MAX_BONES]
) {
	string node_name(node->mName.data);
	const aiNodeAnim* node_anim = find_node_anim(anim_data.animation, node_name);

	mat4 node_transformation = convert_assimp_matrix4(node->mTransformation);
	if (node_anim) {
		vec3 scale = calc_interpolated_scaling(anim_data.animation_time, node_anim);
		mat4 scaleM = S(scale.x, scale.y, scale.z);

		aiQuaternion rot = calc_interpolated_rotation(anim_data.animation_time, node_anim);
		aiMatrix3x3 aiRotM = rot.GetMatrix();
		mat4 rotM = convert_assimp_matrix3(aiRotM);

		vec3 pos = calc_interpolated_position(anim_data.animation_time, node_anim);
		mat4 posM = T(pos.x, pos.y, pos.z);

		node_transformation = posM * rotM * scaleM;
	}

	mat4 global_transform = parent_transform * node_transformation;

	auto& bone_mapping = anim_data.mesh->bone_index_mapping;
	if (bone_mapping.find(node_name) != bone_mapping.end()) {
		uint bone_index = bone_mapping[node_name];
		bone_transforms[bone_index] = global_transform * anim_data.mesh->bone_offsets[bone_index];
	}

	for (uint i = 0; i < node->mNumChildren; i++) {
		read_node_heirarchy(anim_data, node->mChildren[i], global_transform, bone_transforms);
	}
}

vector<Mesh> g_meshes;
vector<GLuint> textures;

aiAnimation* g_animation;
aiNode* g_root_node;

void DisplayWindow()
{
	mat4 m;

	glClearColor(0.2, 0.2, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Cylinder
	glUseProgram(g_shader);
	glBindVertexArray(cylinderModel->vao);
	m = Mult(projectionMatrix, modelViewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(g_shader, "matrix"), 1, GL_TRUE, m.m);

	//DrawCylinder();

	// Guard
	glUseProgram(g_mesh_shader);
	glUniformMatrix4fv(glGetUniformLocation(g_mesh_shader, "matrix"), 1, GL_TRUE, m.m);

	for (Mesh& mesh : g_meshes) {
		glBindVertexArray(mesh.VAO);

		assert(mesh.material_index < textures.size());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[mesh.material_index]);

		glUniform1i(glGetUniformLocation(g_mesh_shader, "sampler"), 0);
		GLuint bone_loc = glGetUniformLocation(g_mesh_shader, "bones");

		float time_in_seconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

		mat4 bone_transforms[MAX_BONES];
		float time_in_ticks = time_in_seconds * g_animation->mTicksPerSecond;
		float animation_time = fmod(time_in_ticks, g_animation->mDuration);

		AnimationData anim_data { animation_time, g_animation, &mesh };
		read_node_heirarchy(anim_data, g_root_node, IdentityMatrix(), bone_transforms);

		// for (uint i = 0; i < MAX_BONES; i++) {
		// 	bone_transforms[i] = IdentityMatrix();
		// }

		glUniformMatrix4fv(bone_loc, MAX_BONES, GL_TRUE, (const GLfloat*)bone_transforms);

		glDrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
	}

	glutSwapBuffers();
};

void OnTimer(int value)
{
	glutPostRedisplay();
	glutTimerFunc(20, &OnTimer, value);
}

void reshape(GLsizei w, GLsizei h)
{
	vec3 cam = { 0, 10, 40 };
	vec3 look = { 0, 15, 0 };

	glViewport(0, 0, w, h);
	GLfloat ratio = (GLfloat) w / (GLfloat) h;
	projectionMatrix = perspective(90, ratio, 0.1, 1000);
	modelViewMatrix = lookAt(
		cam.x, cam.y, cam.z,
		look.x, look.y, look.z,
		0, 1, 0
	);
}

/////////////////////////////////////////
//		M A I N
//
int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2); // Might not be needed in Linux
	glutCreateWindow("Them bones, them bones");

	glutDisplayFunc(DisplayWindow);
	glutTimerFunc(20, &OnTimer, 0);
	glutReshapeFunc(reshape);

	g_shader = loadShaders("shader.vert" , "shader.frag");
	g_mesh_shader = loadShaders("mesh_shader.vert" , "mesh_shader.frag");

	// Set up depth buffer
	glEnable(GL_DEPTH_TEST);

	// initiering
#ifdef WIN32
	glewInit();
#endif

	parse_md5_mesh("boblampclean.md5mesh");

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		"boblampclean.md5mesh",
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs
	);
	for (uint i = 0; i < scene->mNumMeshes; i++) {
		g_meshes.push_back(load_mesh(scene->mMeshes[i]));
	}
	printf("Loaded mesh.\n");

	g_animation = scene->mAnimations[0];
	g_root_node = scene->mRootNode;

	textures.reserve(scene->mNumMaterials);
	for (uint i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* mat = scene->mMaterials[i];

		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;

			if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &path,
								NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				int w, h, n;
				unsigned char* data = stbi_load(path.data, &w, &h, &n, 4);
				if (!data) {
					fprintf(stderr, "Error: could not load %s!\n", path.data);
					printf("Loaded %s\n", path.data);

					// Add something to the vector for index purposes
					textures.push_back(0);
				} else {
					GLuint texID;
					glGenTextures(1, &texID);
					glBindTexture(GL_TEXTURE_2D, texID);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					GLenum type = GL_RGBA;
					glTexImage2D(GL_TEXTURE_2D, 0, type, w, h, 0, type, GL_UNSIGNED_BYTE, data);
					glGenerateMipmap(GL_TEXTURE_2D);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

					stbi_image_free(data);
					textures.push_back(texID);
				}
			}
		}
	}
	printf("Loaded textures.\n");

	BuildCylinder();
	setupBones();
	initBoneWeights();

	glutMainLoop();
	return 0;
}
