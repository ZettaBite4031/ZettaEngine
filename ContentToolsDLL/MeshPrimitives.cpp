#include "MeshPrimitives.h"
#include "Geometry.h"


namespace Zetta::Tools {
	namespace {
		using namespace Math;
		using PrimitiveMeshCreator = void(*)(Scene&, const PrimitiveInitInfo&);

		void CreatePlane(Scene& scene, const PrimitiveInitInfo& info);
		void CreateCube(Scene& scene, const PrimitiveInitInfo& info);
		void CreateUVSphere(Scene& scene, const PrimitiveInitInfo& info);
		void CreateIcosphere(Scene& scene, const PrimitiveInitInfo& info);
		void CreateCylinder(Scene& scene, const PrimitiveInitInfo& info);
		void CreateCapsule(Scene& scene, const PrimitiveInitInfo& info);

		PrimitiveMeshCreator creators[PrimitiveMeshType::count]
			{ CreatePlane, CreateCube, CreateUVSphere, CreateIcosphere, CreateCylinder, CreateCapsule };

		static_assert(_countof(creators) == PrimitiveMeshType::count);

		struct Axis {
			enum : u32 {
				x = 0,
				y = 1,
				z = 2
			};
		};

		Mesh CreatePlane(const PrimitiveInitInfo& info, 
			u32 hIdx = Axis::x, u32 vIdx = Axis::z, bool flip = false,
			v3 offset = { -0.5, 0.f, -0.5f }, v2 u_range = { 0.f, 1.f }, v2 v_range = { 0.f, 1.f }) {

			assert(hIdx < 3 && vIdx < 3);
			assert(hIdx != vIdx);

			const u32 hCount{ clamp(info.segments[hIdx], 1u, 10u) };
			const u32 vCount{ clamp(info.segments[vIdx], 1u, 10u) };
			const f32 horStep{ 1.f / hCount };
			const f32 verStep{ 1.f / vCount };
			const f32 uStep{ (u_range.y - u_range.x) / hCount };
			const f32 vStep{ (v_range.y - v_range.y) / vCount };

			Mesh m{};
			util::vector<v2> uvs;

			for (u32 j{ 0 }; j <= vCount; j++) 
				for (u32 i{ 0 }; i <= hCount; i++) {
					v3 pos{ offset };
					f32* const as_array{ &pos.x };
					as_array[hIdx] += i * horStep;
					as_array[vIdx] += j * verStep;
					m.positions.emplace_back(pos.x * info.size.x, pos.y * info.size.y, pos.z * info.size.z);

					v2 uv{ u_range.x, 1.f - v_range.x };
					uv.x += i * uStep;
					uv.y += i * vStep;
					uvs.emplace_back(uv);
				}
			
			assert(m.positions.size() == (((u64)hCount + 1) * ((u64)vCount + 1)));

			const u32 row_length{ hCount + 1 }; // number of vertices in a row
			for (u32 j{ 0 }; j < vCount; j++) {
				u32 k{ 0 };
				for (u32 i{ k }; i < hCount; i++) {
					const u32 index[4]{
						(i + 0) + (j + 0) * row_length,
						(i + 0) + (j + 1) * row_length,
						(i + 1) + (j + 0) * row_length,
						(i + 1) + (j + 1) * row_length
					};

					m.raw_indices.emplace_back(index[0]);
					m.raw_indices.emplace_back(index[flip ? 2 : 1]);
					m.raw_indices.emplace_back(index[flip ? 1 : 2]);

					m.raw_indices.emplace_back(index[2]);
					m.raw_indices.emplace_back(index[flip ? 3 : 1]);
					m.raw_indices.emplace_back(index[flip ? 1 : 3]);
				}
				k++;
			}

			const u32 num_indices{ 3 * 2 * hCount * vCount };
			assert(m.raw_indices.size() == num_indices);
			m.uv_sets.resize(1);
			for (u32 i{ 0 }; i < num_indices; i++) 
				m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
			
			return m;
		}

		void CreatePlane(Scene& scene, const PrimitiveInitInfo& info){
			LODGroup lod{};
			lod.name = "plane";
			lod.meshes.emplace_back(CreatePlane(info));
			scene.lod_groups.emplace_back(lod);
		}

		void CreateCube(Scene& scene, const PrimitiveInitInfo& info){

		}

		void CreateUVSphere(Scene& scene, const PrimitiveInitInfo& info){

		}

		void CreateIcosphere(Scene& scene, const PrimitiveInitInfo& info) {

		}

		void CreateCylinder(Scene& scene, const PrimitiveInitInfo& info){

		}

		void CreateCapsule(Scene& scene, const PrimitiveInitInfo& info){

		}

	}

	EDITOR_INTERFACE void CreatePrimitiveMesh(SceneData* data, PrimitiveInitInfo* info) {
		assert(data && info);
		assert(info->type < PrimitiveMeshType::count);
		Scene scene{};
		creators[info->type](scene, *info);

		data->settings.calculate_normals = 1;
		ProcessScene(scene, data->settings);
		PackData(scene, *data);
	}
}