#include "MeshPrimitives.h"
#include "Geometry.h"


namespace Zetta::Tools {
	namespace {
		using namespace Math;
        using namespace DirectX;
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
            u32 horizontal_index = Axis::x, u32 vertical_index = Axis::z, bool flip_winding = false,
            v3 offset = { -0.5f, 0.f, -0.5f }, v2 u_range = { 0.f, 1.f }, v2 v_range = { 0.f, 1.f })
        {
            assert(horizontal_index < 3 && vertical_index < 3);
            assert(horizontal_index != vertical_index);

            const u32 horizontal_count{ clamp(info.segments[horizontal_index], 1u, 10u) };
            const u32 vertical_count{ clamp(info.segments[vertical_index], 1u, 10u) };
            const f32 horizontal_step{ 1.f / horizontal_count };
            const f32 vertical_step{ 1.f / vertical_count };
            const f32 u_step{ (u_range.y - u_range.x) / horizontal_count };
            const f32 v_step{ (v_range.y - v_range.x) / vertical_count };

            Mesh m{};
            util::vector<v2> uvs;

            for (u32 j{ 0 }; j <= vertical_count; ++j)
                for (u32 i{ 0 }; i <= horizontal_count; ++i)
                {
                    v3 position{ offset };
                    f32* const as_array{ &position.x };
                    as_array[horizontal_index] += i * horizontal_step;
                    as_array[vertical_index] += j * vertical_step;
                    m.positions.emplace_back(position.x * info.size.x, position.y * info.size.y, position.z * info.size.z);

                    v2 uv{ u_range.x, 1.f - v_range.x };
                    uv.x += i * u_step;
                    uv.y -= j * v_step;
                    uvs.emplace_back(uv);
                }

            assert(m.positions.size() == (((u64)horizontal_count + 1) * ((u64)vertical_count + 1)));

            const u32 row_length{ horizontal_count + 1 }; // number of vertices in a row
            for (u32 j{ 0 }; j < vertical_count; ++j)
            {
                for (u32 i{ 0 }; i < horizontal_count; ++i)
                {
                    const u32 index[4]
                    {
                        i + j * row_length,
                        i + (j + 1) * row_length,
                        (i + 1) + j * row_length,
                        (i + 1) + (j + 1) * row_length
                    };

                    m.raw_indices.emplace_back(index[0]);
                    m.raw_indices.emplace_back(index[flip_winding ? 2 : 1]);
                    m.raw_indices.emplace_back(index[flip_winding ? 1 : 2]);

                    m.raw_indices.emplace_back(index[2]);
                    m.raw_indices.emplace_back(index[flip_winding ? 3 : 1]);
                    m.raw_indices.emplace_back(index[flip_winding ? 1 : 3]);
                }
            }

            const u32 num_indices{ 3 * 2 * horizontal_count * vertical_count };
            assert(m.raw_indices.size() == num_indices);

            m.uv_sets.resize(1);

            for (u32 i{ 0 }; i < num_indices; ++i)
            {
                m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
            }

            return m;
		}

        Mesh CreateUVSphere(const PrimitiveInitInfo& info) {
            const u32 phi_count{ clamp(info.segments[Axis::x], 3u, 64u) };
            const u32 theta_count{ clamp(info.segments[Axis::y], 2u, 64u) };
            const f32 theta_step{ PI / theta_count };
            const f32 phi_step{ TAU / phi_count };
            const u32 num_indices{ 2 * 3 * phi_count + 2 * 3 * phi_count * (theta_count - 2) };
            const u32 num_vertices{ 2 + phi_count * (theta_count - 1) };

            Mesh m{};
            m.name = "uv_sphere";
            m.positions.resize(num_vertices);

            u32 c{ 0 };
            m.positions[c++] = { 0.f, info.size.y, 0.f };

            for (u32 j{ 1 }; j <= (theta_count - 1); j++) {
                const f32 theta{ j * theta_step };

                for (u32 i{ 0 }; i < phi_count; i++) {
                    const f32 phi{ i * phi_step };
                    m.positions[c++] = {
                        info.size.x * XMScalarSin(theta) * XMScalarCos(phi),
                        info.size.y * XMScalarCos(theta),
                        -info.size.z * XMScalarSin(theta) * XMScalarSin(phi) };
                }

            }
            
            m.positions[c++] = { 0.f, -info.size.y, 0.f };
            assert(num_vertices == c);
        
            c = 0;
            m.raw_indices.resize(num_indices);
            util::vector<v2> uvs(num_indices);
            const f32 inv_theta_count{ 1.f / theta_count };
            const f32 inv_phi_count{ 1.f / phi_count };

            for (u32 i{ 0 }; i < phi_count - 1; i++) {
                uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 1.f };
                m.raw_indices[c++] = 0;
                uvs[c] = { i * inv_phi_count, 1.f - inv_theta_count };
                m.raw_indices[c++] = i+1;
                uvs[c] = { (i + 1) * inv_phi_count, 1.f - inv_theta_count };
                m.raw_indices[c++] = i+2;

            }

            uvs[c] = { 1.f - 0.5f * inv_phi_count, 1.f };
            m.raw_indices[c++] = 0;
            uvs[c] = { 1.f - inv_phi_count, 1.f - inv_theta_count };
            m.raw_indices[c++] = phi_count;
            uvs[c] = { 1.f, 1.f - inv_theta_count };
            m.raw_indices[c++] = 1;

            for (u32 j{ 0 }; j < (theta_count - 2); j++) {
                for (u32 i{ 0 }; i < (phi_count - 1); i++) {
                    const u32 index[4]{
                        1 + (i + 0) +(j + 0) * phi_count,
                        1 + (i + 0) +(j + 1) * phi_count,
                        1 + (i + 1) +(j + 1) * phi_count,
                        1 + (i + 1) +(j + 0) * phi_count,
                    };

                    uvs[c] = { i * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
                    m.raw_indices[c++] = index[0];
                    uvs[c] = { i * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
                    m.raw_indices[c++] = index[1];
                    uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
                    m.raw_indices[c++] = index[2];

                    uvs[c] = { i * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
                    m.raw_indices[c++] = index[0];
                    uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
                    m.raw_indices[c++] = index[2];
                    uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
                    m.raw_indices[c++] = index[3];
                }

                const u32 index[4]{
                    0 + phi_count + (j + 0) *phi_count,
                    0 + phi_count + (j + 1) * phi_count,
                    1 + (j + 1) * phi_count,
                    1 + (j + 0) *phi_count,
                };

                uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count };
                m.raw_indices[c++] = index[0];
                uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 2) * inv_theta_count };
                m.raw_indices[c++] = index[1];
                uvs[c] = { 1.f, 1.f - (j + 2) * inv_theta_count };
                m.raw_indices[c++] = index[2];

                uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count };
                m.raw_indices[c++] = index[0];
                uvs[c] = { 1.f, 1.f - (j + 2) * inv_theta_count };
                m.raw_indices[c++] = index[2];
                uvs[c] = { 1.f, 1.f - (j + 1) * inv_theta_count };
                m.raw_indices[c++] = index[3];
            }

            const u32 south_pole_index{ (u32)m.positions.size() - 1 };
            for (u32 i{ 0 }; i < (phi_count - 1); i++) {
                uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 0.f };
                m.raw_indices[c++] = south_pole_index;
                uvs[c] = { (i + 1) * inv_phi_count, inv_theta_count };
                m.raw_indices[c++] = south_pole_index - phi_count + i + 1;
                uvs[c] = { i * inv_phi_count, inv_theta_count };
                m.raw_indices[c++] = south_pole_index - phi_count + i;
            }

            uvs[c] = { 1.f - 0.5f * inv_phi_count, 0.f };
            m.raw_indices[c++] = south_pole_index;
            uvs[c] = { 1.f, inv_theta_count };
            m.raw_indices[c++] = south_pole_index - phi_count;
            uvs[c] = { 1.f - inv_phi_count, inv_theta_count };
            m.raw_indices[c++] = south_pole_index - 1;

            assert(c == num_indices);
            m.uv_sets.emplace_back(uvs);

            return m;
        }

		void CreatePlane(Scene& scene, const PrimitiveInitInfo& info){
			LODGroup lod{};
			lod.name = "plane";
			lod.meshes.emplace_back(CreatePlane(info));
			scene.lod_groups.emplace_back(lod);
		}

		void CreateCube(Scene&, const PrimitiveInitInfo&){
            
		}

		void CreateUVSphere(Scene& scene, const PrimitiveInitInfo& info){
            LODGroup lod{};
            lod.name = "uv_sphere";
            lod.meshes.emplace_back(CreateUVSphere(info));
            scene.lod_groups.emplace_back(lod);
		}

		void CreateIcosphere(Scene&, const PrimitiveInitInfo&) {

		}

		void CreateCylinder(Scene&, const PrimitiveInitInfo&){

		}

		void CreateCapsule(Scene&, const PrimitiveInitInfo&){

		}

	}

	EDITOR_INTERFACE void CreatePrimitiveMesh(SceneData* data, PrimitiveInitInfo* info) {
		assert(data && info);
		assert(info->type < PrimitiveMeshType::count);
		Scene scene{};
		creators[info->type](scene, *info);

        Progression progression{};
        ProcessScene(scene, data->settings, &progression);
		PackData(scene, *data);
	}
}