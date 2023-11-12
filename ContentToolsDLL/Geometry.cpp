#include "Geometry.h"

namespace Zetta::Tools {
	namespace {
		using namespace Math;
		using namespace DirectX;
		void RecalculateNormals(Mesh& m) {
			const u32 num_indices{ (u32)m.raw_indices.size() };
			m.normals.reserve(num_indices);

			for (u32 i{ 0 }; i < num_indices; i++) {
				const u32 i0{ m.raw_indices[i] };
				const u32 i1{ m.raw_indices[i++] };
				const u32 i2{ m.raw_indices[i++] };

				XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
				XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
				XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

				XMVECTOR e0{ v1 - v0 };
				XMVECTOR e1{ v2 - v0 };
				XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

				XMStoreFloat3(&m.normals[i], n);
				m.normals[i - 1] = m.normals[i];
				m.normals[i - 2] = m.normals[i];
			}
		}

		void ProcessNormals(Mesh& m, float smoothing_angle) {
			const f32 cos_alpha{ XMScalarCos(Math::PI - smoothing_angle * Math::PI / 180.0f) };
			const bool is_hard_edge{ XMScalarNearEqual(smoothing_angle, 180.f, Math::EPSILON) };
			const bool is_soft_edge{ XMScalarNearEqual(smoothing_angle, 0.f, Math::EPSILON) };
			const u32 num_indices{ (u32)m.raw_indices.size() };
			const u32 num_vertices{ (u32)m.positions.size() };
			assert(num_indices && num_vertices);

			m.indices.resize(num_indices);

			util::vector<util::vector<u32>> idx_ref(num_vertices);
			for (u32 i{ 0 }; i < num_indices; i++)
				idx_ref[m.raw_indices[i]].emplace_back(i);

			for (u32 i{ 0 }; i < num_vertices; i++) {
				auto& refs{ idx_ref[i] };
				u32 num_refs{ (u32)refs.size() };
				for (u32 j{ 0 }; j < num_refs; j++) {
					m.indices[refs[j]] = (u32)m.vertices.size();
					Vertex& v{ m.vertices.emplace_back() };
					v.position = m.positions[m.raw_indices[refs[j]]];

					XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
					if (!is_hard_edge) {
						for (u32 k{ j + 1 }; k < num_refs; k++) {
							f32 cos_theta{ 0.f }; 
							XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
							if (!is_soft_edge) {
								XMStoreFloat(&cos_theta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
							}

							if (is_soft_edge || cos_theta >= cos_alpha) {
								n1 += n2;

								m.indices[refs[k]] = m.indices[refs[j]];
								refs.erase(refs.begin() + k);
								num_refs--;
								k--;
							}
						}
					}
					XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
				}
			}
		}

		void ProcessUVs(Mesh& m) {
			util::vector<Vertex> old_vertices;
			old_vertices.swap(m.vertices);
			util::vector<u32> old_indices(m.indices.size());
			old_indices.swap(m.indices);

			const u32 num_vertices{ (u32)old_vertices.size() };
			const u32 num_indices{ (u32)old_indices.size() };
			assert(num_vertices && num_indices);

			util::vector<util::vector<u32>> idx_ref(num_vertices);
			for (u32 i{ 0 }; i < num_indices; i++)
				idx_ref[old_indices[i]].emplace_back(i);

			for (u32 i{ 0 }; i < num_indices; i++) {
				auto& refs{ idx_ref[i] };
				u32 num_refs{ (u32)refs.size() };
				for (u32 j{ 0 }; j < num_refs; j++) {
					m.indices[refs[j]] = (u32)m.vertices.size();
					Vertex& v{ old_vertices[old_indices[refs[j]]] };
					v.uv = m.uv_sets[0][refs[j]];
					m.vertices.emplace_back(v);

					for (u32 k{ 0 }; k < num_refs; k++) {
						Math::v2& uv1{ m.uv_sets[0][refs[k]] };
						if (XMScalarNearEqual(v.uv.x, uv1.x, Math::EPSILON) &&
							XMScalarNearEqual(v.uv.y, uv1.y, Math::EPSILON)) {
							m.indices[refs[k]] = m.indices[refs[j]];
							refs.erase(refs.begin() + k);
							num_refs--;
							k--;
						}
					}
				}
			}
		}

		void PackVerticesStatic(Mesh& m) {
			const u32 num_vertices{ (u32)m.vertices.size() };
			assert(num_vertices);
			m.packed_vertices_static.reserve(num_vertices);

			for (u32 i{ 0 }; i < num_vertices; i++) {
				Vertex& v{ m.vertices[i] };
				const u8 signs{ (u8)((v.normal.z > 0.f) << 1) };
				const u16 normal_x{ (u16)PackFloat<16>(v.normal.x, -1.f, 1.f) };
				const u16 normal_y{ (u16)PackFloat<16>(v.normal.y, -1.f, 1.f) };

				m.packed_vertices_static
					.emplace_back(PackedVertex::VertexStatic{
						v.position, {0,0,0}, signs,
						{normal_x, normal_y}, {},
						v.uv });
			}
		}

		void ProcessVertices(Mesh& m, const GeometryImportSettings& settings) {
			assert((m.raw_indices.size() % 3) == 0);
			if (settings.calculate_normals || m.normals.empty()) 
				RecalculateNormals(m);
			
			ProcessNormals(m, settings.smoothing_angle);

			if (!m.uv_sets.empty())
				ProcessUVs(m);

			PackVerticesStatic(m);
		}

		u64 GetMeshSize(const Mesh& m) {
			const u64 num_vertices{ m.vertices.size() };
			const u64 vertex_buffer_size{ sizeof(PackedVertex::VertexStatic) * num_vertices };
			const u64 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
			const u64 index_buffer_size{ index_size * m.indices.size() };
			constexpr u64  su32{ sizeof(u32) };
			const u64 size{
				su32 + m.name.size() + // mesh name length and room for mesh name string
				su32 + // lod id
				su32 + // vertex size
				su32 + // number of vertices
				su32 + // index size (16 or 32 bit)
				su32 + // number of indices
				sizeof(f32) + // LOD threshold
				vertex_buffer_size + // room for vertices
				index_buffer_size // room for indices
			};

			return size;
		}

		u64 GetSceneSize(const Scene& scene) {
			constexpr u64 su32{ sizeof(u32) };
			u64 size{ su32 + scene.name.size() + su32 };

			for (auto& lod : scene.lod_groups) {
				u64 lod_size{ su32 + lod.name.size() + su32 };
				for (auto& m : lod.meshes) lod_size += GetMeshSize(m);
				size += lod_size;
			}

			return size;
		}

		void PackMeshData(const Mesh& m, u8* buffer, u64& at) {
			constexpr u64 su32{ sizeof(u32) };
			u32 s{ 0 };

			s = (u32)m.name.size();
			memcpy(&buffer[at], &s, su32); at += su32;
			memcpy(&buffer[at], m.name.c_str(), s); at += s;

			s = m.lod_id;
			memcpy(&buffer[at], &s, su32); at += su32;

			constexpr u32 vertex_size{ sizeof(PackedVertex::VertexStatic) };
			s = vertex_size;
			memcpy(&buffer[at], &s, su32); at += su32;

			const u32 num_vertices{ (u32)m.vertices.size() };
			s = num_vertices;
			memcpy(&buffer[at], &s, su32); at += su32;

			const u32 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
			s = index_size;
			memcpy(&buffer[at], &s, su32); at += su32;

			const u32 num_indices{ (u32)m.indices.size() };
			s = num_indices;
			memcpy(&buffer[at], &s, su32); at += su32;

			memcpy(&buffer[at], &m.lod_threshold, sizeof(f32)); at += sizeof(f32);

			s = vertex_size * num_vertices;
			memcpy(&buffer[at], m.packed_vertices_static.data(), s); at += s;

			s = index_size * num_indices;
			void* data{ (void*)m.indices.data() };
			util::vector<u16> indices;

			if (index_size == sizeof(u16)) {
				indices.resize(num_indices);
				for (u32 i{ 0 }; i < num_indices; i++) indices[i] = (u16)m.indices[i];
				data = (void*)indices.data();
			}
			memcpy(&buffer[at], data, s); at += s;
		}
	}

	void ProcessScene(Scene& scene, const GeometryImportSettings& settings) {
		for (auto& lod : scene.lod_groups) 
			for (auto& m : lod.meshes) 
				ProcessVertices(m, settings);
	}

	void PackData(const Scene& scene, SceneData& data) {
		constexpr u64 su32{ sizeof(u32) };
		const u64 scene_size{ GetSceneSize(scene) };
		data.size = (u32)scene_size;
		data.data = (u8*)CoTaskMemAlloc(scene_size);
		assert(data.data);

		u8* const buffer{ data.data };
		u64 at{ 0 };
		u32 s{ 0 };

		s = (u32)scene.name.size();
		memcpy(&buffer[at], &s, su32); at += su32;
		memcpy(&buffer[at], scene.name.c_str(), s); at += s;

		s = (u32)scene.lod_groups.size();
		memcpy(&buffer[at], &s, su32); at += su32;

		for (auto& lod : scene.lod_groups) {
			s = (u32)lod.name.size();
			memcpy(&buffer[at], &s, su32); at += su32;
			memcpy(&buffer[at], lod.name.c_str(), s); at += s;

			s = (u32)lod.meshes.size();
			memcpy(&buffer[at], &s, su32); at += su32;

			for (auto& m : lod.meshes)
				PackMeshData(m, buffer, at);
		}
	}
}
