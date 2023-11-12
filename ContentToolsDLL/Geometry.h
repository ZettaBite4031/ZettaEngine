#pragma once
#include "ToolsCommon.h"

namespace Zetta::Tools {
	namespace PackedVertex {
		struct VertexStatic {
			Math::v3	position;
			u8			reserved[3];
			u8			t_sign;
			u16			norma[2];
			u16			tangent[2];
			Math::v2	uv;
		};

	}

	struct Vertex {
		Math::v4 tangent{};
		Math::v3 position{};
		Math::v3 normal{};
		Math::v2 uv{};
	};
	
	struct Mesh {
		// Initial data
		util::vector<Math::v3>					positions;
		util::vector<Math::v3>					normals;
		util::vector<Math::v4>					tangents;
		util::vector<util::vector<Math::v2>>	uv_sets;

		util::vector<u32>						raw_indices;

		// Intermediate data
		util::vector<Vertex>					vertices;
		util::vector<u32>						indices;

		// Output data
		std::string									name;
		util::vector<PackedVertex::VertexStatic>	packed_vertices_static;
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