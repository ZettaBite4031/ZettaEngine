#pragma once
#include "ToolsCommon.h"

namespace Zetta::Tools {
	struct Vertex {
		Math::v4 tangent{};
		Math::v4 joint_weights{};
		Math::u32v4 joint_indices{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
		Math::v3 position{};
		Math::v3 normal{};
		Math::v2 uv{};
		u8 red{}, green{}, blue{};
		u8 pad;
	};
	
	namespace Elements {
		struct ElementType {
			enum Type : u32 {
				PositionOnly = 0x00,
				StaticNormal = 0x01,
				StaticNormalTexture = 0x03,
				StaticColor = 0x04,
				Skeletal = 0x08,
				SkeletalColor = Skeletal | StaticColor,
				SkeletalNormal = Skeletal | StaticNormal,
				SkeletalNormalColor = SkeletalNormal | StaticColor,
				SkeletalNormalTexture = Skeletal | StaticNormalTexture,
				SkeletalNormalTextureColor = SkeletalNormalTexture | StaticColor
			};
		};

		struct StaticColor {
			u8 color[3];
			u8 pad;
		};

		struct StaticNormal {
			u8 color[3];
			u8 t_sign;
			u16 normal[2];
		};

		struct StaticNormalTexture {
			u8 color[3];
			u8 t_sign;
			u16 normal[2];
			u16 tangent[2];
			Math::v2 uv;
		};

		struct Skeletal {
			u8 joint_weights[3];
			u8 pad;
			u16 joint_indices[4];
		};

		struct SkeletalColor {
			u8 joint_weights[3];
			u8 pad;
			u16 joint_indices[4];
			u8 color[3];
			u8 pad2;
		};

		struct SkeletalNormal {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indices[4];
			u16 normal[2];
		};

		struct SkeletalNormalColor {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indices[4];
			u16 normal[2];
			u8 color[3];
			u8 pad;
		};

		struct SkeletalNormalTexture {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indices[4];
			u16 normal[2];
			u16 tangent[2];
			Math::v2 uv;
		};

		struct SkeletalNormalTextureColor {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indices[4];
			u16 normal[2];
			u16 tangent[2];
			Math::v2 uv;
			u8 color[3];
			u8 pad;
		};
	}

	struct Mesh {
		// Initial data
		util::vector<Math::v3>					positions;
		util::vector<Math::v3>					normals;
		util::vector<Math::v3>					colors;
		util::vector<Math::v4>					tangents;
		util::vector<util::vector<Math::v2>>	uv_sets;
		util::vector<u32>						material_indices;
		util::vector<u32>						material_used;

		util::vector<u32>						raw_indices;

		// Intermediate data
		util::vector<Vertex>					vertices;
		util::vector<u32>						indices;

		// Output data
		std::string									name;
		Elements::ElementType::Type					elements_type;
		util::vector<u8>							positions_buffer;
		util::vector<u8>							element_buffer;

		f32											lod_threshold{ -1.f };
		u32											lod_id{ u32_invalid_id };
	};
	
	struct LODGroup {
		std::string			name;
		util::vector<Mesh>	meshes;
	};

	struct Scene {
		std::string					name;
		util::vector<LODGroup>		lod_groups;
	};

	struct GeometryImportSettings {
		f32 smoothing_angle;
		u8	calculate_normals;
		u8	calculate_tangents;
		u8	reverse_handedness;
		u8	import_embeded;
		u8	import_animations;
	};

	struct SceneData {
		u8* data;
		u32 size;
		GeometryImportSettings settings;
	};

	void ProcessScene(Scene& scene, const GeometryImportSettings& settings);
	void PackData(const Scene& scene, SceneData& data);
}