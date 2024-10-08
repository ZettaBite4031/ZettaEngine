#include "Geometry.h"
#include "../Utilities/IOStream.h"

namespace Zetta::Tools {
    namespace {

        using namespace Math;
        using namespace DirectX;

        void RecalculateNormals(Mesh& m)
        {
            const u32 num_indices{ (u32)m.raw_indices.size() };
            m.normals.resize(num_indices);

            for (u32 i{ 0 }; i < num_indices; ++i)
            {
                const u32 i0{ m.raw_indices[i] };
                const u32 i1{ m.raw_indices[++i] };
                const u32 i2{ m.raw_indices[++i] };

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

        void ProcessNormals(Mesh& m, f32 smoothing_angle)
        {
            const f32 cos_alpha{ XMScalarCos(PI - smoothing_angle * PI / 180.f) };
            const bool is_hard_edge{ XMScalarNearEqual(smoothing_angle, 180.f, EPSILON) };
            const bool is_soft_edge{ XMScalarNearEqual(smoothing_angle, 0.f, EPSILON) };
            const u32 num_indices{ (u32)m.raw_indices.size() };
            const u32 num_vertices{ (u32)m.positions.size() };
            assert(num_indices && num_vertices);

            m.indices.resize(num_indices);

            util::vector<util::vector<u32>> idx_ref(num_vertices);
            for (u32 i{ 0 }; i < num_indices; ++i)
                idx_ref[m.raw_indices[i]].emplace_back(i);

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < num_refs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    Vertex& v{ m.vertices.emplace_back() };
                    v.position = m.positions[m.raw_indices[refs[j]]];

                    XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
                    if (!is_hard_edge)
                    {
                        for (u32 k{ j + 1 }; k < num_refs; ++k)
                        {
                            // this value represents the cosine of the angle between normals.
                            f32 cos_theta{ 0.f };
                            u32& r = refs[k];
                            XMVECTOR n2{ XMLoadFloat3(&m.normals[r]) };
                            if (!is_soft_edge)
                            {
                                // NOTE: we're accounting for the length of n1 in this calculation because
                                //       it can possibly change in this loop iteration. We assume unit length
                                //       for n2. 
                                //       cos(angle) = dot(n1, n2) / (||n1||*||n2||)
                                XMStoreFloat(&cos_theta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
                            }

                            if (is_soft_edge || cos_theta >= cos_alpha)
                            {
                                n1 += n2;

                                m.indices[refs[k]] = m.indices[refs[j]];
                                refs.erase(refs.begin() + k);
                                --num_refs;
                                --k;
                            }
                        }
                    }
                    XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
                }
            }
        }

        void ProcessUVs(Mesh& m)
        {
            util::vector<Vertex> old_vertices;
            old_vertices.swap(m.vertices);
            util::vector<u32> old_indices(m.indices.size());
            old_indices.swap(m.indices);

            const u32 num_vertices{ (u32)old_vertices.size() };
            const u32 num_indices{ (u32)old_indices.size() };
            assert(num_vertices && num_indices);

            util::vector<util::vector<u32>> idx_ref(num_vertices);
            for (u32 i{ 0 }; i < num_indices; ++i)
                idx_ref[old_indices[i]].emplace_back(i);

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < num_refs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    Vertex& v{ old_vertices[old_indices[refs[j]]] };
                    v.uv = m.uv_sets[0][refs[j]];
                    m.vertices.emplace_back(v);

                    for (u32 k{ j + 1 }; k < num_refs; ++k)
                    {
                        v2& uv1{ m.uv_sets[0][refs[k]] };
                        if (XMScalarNearEqual(v.uv.x, uv1.x, EPSILON) &&
                            XMScalarNearEqual(v.uv.y, uv1.y, EPSILON))
                        {
                            m.indices[refs[k]] = m.indices[refs[j]];
                            refs.erase(refs.begin() + k);
                            --num_refs;
                            --k;
                        }
                    }
                }
            }
        }

        u64 GetVertexElementSize(Elements::ElementsType::Type elements_type) {
            using namespace Elements;
            switch (elements_type)
            {
            case ElementsType::StaticNormal:                 return sizeof(StaticNormal);
            case ElementsType::StaticNormalTexture:          return sizeof(StaticNormalTexture);
            case ElementsType::StaticColor:                  return sizeof(StaticColor);
            case ElementsType::Skeletal:                     return sizeof(Skeletal);
            case ElementsType::SkeletalColor:                return sizeof(SkeletalColor);
            case ElementsType::SkeletalNormal:               return sizeof(SkeletalNormal);
            case ElementsType::SkeletalNormalColor:          return sizeof(SkeletalNormalColor);
            case ElementsType::SkeletalNormalTexture:        return sizeof(SkeletalNormalTexture);
            case ElementsType::SkeletalNormalTextureColor:   return sizeof(SkeletalNormalTextureColor);
            }

            return 0;
        }

        void PackVertices(Mesh& m) {
            const u32 num_vertices{ (u32)m.vertices.size() };
            assert(num_vertices);

            m.positions_buffer.resize(sizeof(Math::v3) * num_vertices);
            Math::v3* const positions_buffer{ (Math::v3* const)m.positions_buffer.data() };

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                positions_buffer[i] = m.vertices[i].position;
            }

            struct u16v2 { u16 x, y; };
            struct u8v3 { u8 x, y, z; };

            util::vector<u8> t_signs(num_vertices);
            util::vector<u16v2> normals(num_vertices);
            util::vector<u16v2> tangents(num_vertices);
            util::vector<u8v3> joint_weights(num_vertices);

            if (m.elements_type & Elements::ElementsType::StaticNormal)
            {
                // normals only
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    t_signs[i] = (u8)((v.normal.z > 0.f) << 1);
                    normals[i] = { (u16)PackFloat<16>(v.normal.x, -1.f, 1.f), (u16)PackFloat<16>(v.normal.y, -1.f, 1.f) };
                }

                if (m.elements_type & Elements::ElementsType::StaticNormalTexture)
                {
                    // full T-space
                    for (u32 i{ 0 }; i < num_vertices; ++i)
                    {
                        Vertex& v{ m.vertices[i] };
                        t_signs[i] |= (u8)((v.tangent.w > 0.f) && (v.tangent.z > 0.f));
                        tangents[i] = { (u16)PackFloat<16>(v.tangent.x, -1.f, 1.f), (u16)PackFloat<16>(v.tangent.y, -1.f, 1.f) };
                    }
                }
            }

            if (m.elements_type & Elements::ElementsType::Skeletal)
            {
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    // pack joint weights (from [0.0, 1.0] to [0..255])
                    joint_weights[i] = {
                        (u8)PackUnitFloat<8>(v.joint_weights.x),
                        (u8)PackUnitFloat<8>(v.joint_weights.y),
                        (u8)PackUnitFloat<8>(v.joint_weights.z) };

                    // NOTE: w3 will be calculated in shader since joint weights sum to one(1).
                }
            }

            m.element_buffer.resize(GetVertexElementSize(m.elements_type) * num_vertices);
            using namespace Elements;

            switch (m.elements_type)
            {
            case ElementsType::StaticColor:
            {
                StaticColor* const element_buffer{ (StaticColor* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    element_buffer[i] = { {v.red, v.green, v.blue}, {} };
                }
            }
            break;
            case ElementsType::StaticNormal:
            {
                StaticNormal* const element_buffer{ (StaticNormal* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    element_buffer[i] = { {v.red, v.green, v.blue}, t_signs[i], {normals[i].x, normals[i].y} };
                }
            }
            break;
            case ElementsType::StaticNormalTexture:
            {
                StaticNormalTexture* const element_buffer{ (StaticNormalTexture* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    element_buffer[i] = { {v.red, v.green, v.blue}, t_signs[i],
                                         {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y},
                                         v.uv };
                }
            }
            break;
            case ElementsType::Skeletal:
            {
                Skeletal* const element_buffer{ (Skeletal* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, {},
                                         {indices[0], indices[1], indices[2], indices[3]} };
                }
            }
            break;
            case ElementsType::SkeletalColor:
            {
                SkeletalColor* const element_buffer{ (SkeletalColor* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, {},
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {v.red, v.green, v.blue}, {} };
                }
            }
            break;
            case ElementsType::SkeletalNormal:
            {
                SkeletalNormal* const element_buffer{ (SkeletalNormal* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, t_signs[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y} };
                }
            }
            break;
            case ElementsType::SkeletalNormalColor:
            {
                SkeletalNormalColor* const element_buffer{ (SkeletalNormalColor* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, t_signs[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {v.red, v.green, v.blue}, {} };
                }
            }
            break;
            case ElementsType::SkeletalNormalTexture:
            {
                SkeletalNormalTexture* const element_buffer{ (SkeletalNormalTexture* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, t_signs[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y}, v.uv };
                }
            }
            break;
            case ElementsType::SkeletalNormalTextureColor:
            {
                SkeletalNormalTextureColor* const element_buffer{ (SkeletalNormalTextureColor* const)m.element_buffer.data() };
                for (u32 i{ 0 }; i < num_vertices; ++i)
                {
                    Vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.joint_indices.x, (u16)v.joint_indices.y , (u16)v.joint_indices.z , (u16)v.joint_indices.w };
                    element_buffer[i] = { {joint_weights[i].x, joint_weights[i].y, joint_weights[i].z}, t_signs[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y}, v.uv,
                                         {v.red, v.green, v.blue}, {} };
                }
            }
            break;
            }
        }

        Elements::ElementsType::Type DetermineElementType(const Mesh& m) {
            using namespace Elements;
            Elements::ElementsType::Type type;
            if (m.normals.size()) {
                if (m.uv_sets.size() && m.uv_sets[0].size())
                    type = ElementsType::StaticNormalTexture;
                else type = ElementsType::StaticNormal;
            }
            else if (m.colors.size()) type = ElementsType::StaticColor;
            
            // TODO: Implement Skeletal meshes when the data is available.
            
            return type;
        }

        void ProcessVertices(Mesh& m, const GeometryImportSettings& settings)
        {
            assert((m.raw_indices.size() % 3) == 0);
            if (settings.calculate_normals || m.normals.empty())
                RecalculateNormals(m);
            
            ProcessNormals(m, settings.smoothing_angle);

            if (!m.uv_sets.empty())
                ProcessUVs(m);
            
            m.elements_type = DetermineElementType(m);
            PackVertices(m);
        }

        u64 GetMeshSize(const Mesh& m)
        {
            const u64 num_vertices{ m.vertices.size() };
            const u64 positions_buffer_size{ m.positions_buffer.size() };
            assert(positions_buffer_size == sizeof(Math::v3) * num_vertices);
            const u64 element_buffer_size{ m.element_buffer.size() };
            assert(element_buffer_size == GetVertexElementSize(m.elements_type) * num_vertices);
            const u64 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            const u64 index_buffer_size{ index_size * m.indices.size() };
            constexpr u64 su32{ sizeof(u32) };
            const u64 size{
                su32 + m.name.size() + // Mesh name length and room for Mesh name string
                su32 + // lod id
                su32 + // vertex element size (vertex size excluding position element)
                su32 + // element type enumeration size
                su32 + // number of vertices
                su32 + // index size (16 bit or 32 bit)
                su32 + // number of indices
                sizeof(f32) + // LOD threshold
                positions_buffer_size + // room for vertex positions
                element_buffer_size + // room for vertex Elements
                index_buffer_size // room for indices
            };

            return size;
        }

        u64 GetSceneSize(const Scene& scene)
        {
            constexpr u64 su32{ sizeof(u32) };
            u64 size
            {
                su32 +              // name length
                scene.name.size() + // room for scene name string
                su32                // number of LODs
            };

            for (auto& lod : scene.lod_groups)
            {
                u64 lod_size
                {
                    su32 + lod.name.size() + // LOD name length and room for LPD name string
                    su32                     // number of Meshes in this LOD
                };

                for (auto& m : lod.meshes)
                    lod_size += GetMeshSize(m);
                

                size += lod_size;
            }

            return size;
        }

        void PackMeshData(const Mesh& m, util::BlobStreamWriter& blob)
        {
            // mesh name
            blob.write((u32)m.name.size());
            blob.write(m.name.c_str(), m.name.size());
            // lod id
            blob.write(m.lod_id);
            // vertex element size
            const u32 elements_size{ (u32)GetVertexElementSize(m.elements_type) };
            blob.write(elements_size);
            // Elements type enumeration
            blob.write((u32)m.elements_type);
            // number of vertices
            const u32 num_vertices{ (u32)m.vertices.size() };
            blob.write(num_vertices);
            // index size (16 bit or 32 bit)
            const u32 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            blob.write(index_size);
            // number of indices
            const u32 num_indices{ (u32)m.indices.size() };
            blob.write(num_indices);
            // LOD threshold
            blob.write(m.lod_threshold);
            // position buffer
            assert(m.positions_buffer.size() == sizeof(Math::v3) * num_vertices);
            blob.write(m.positions_buffer.data(), m.positions_buffer.size());
            // element buffer
            assert(m.element_buffer.size() == elements_size * num_vertices);
            blob.write(m.element_buffer.data(), m.element_buffer.size());
            // index data
            const u32 index_buffer_size{ index_size * num_indices };
            const u8* data{ (const u8*)m.indices.data() };
            util::vector<u16> indices;

            if (index_size == sizeof(u16))
            {
                indices.resize(num_indices);
                for (u32 i{ 0 }; i < num_indices; ++i) indices[i] = (u16)m.indices[i];
                data = (const u8*)indices.data();
            }
            blob.write(data, index_buffer_size);
        }

        bool SplitMeshesByMaterial(u32 material_idx, const Mesh& m, Mesh& submesh) {
            submesh.name = m.name;
            submesh.lod_threshold = m.lod_threshold;
            submesh.lod_id = m.lod_id;
            submesh.material_used.emplace_back(material_idx);
            submesh.uv_sets.resize(m.uv_sets.size());

            const u32 num_polys{ (u32)m.raw_indices.size() / 3 };
            util::vector<u32> vertex_ref(m.positions.size(), u32_invalid_id);

            for (u32 i{ 0 }; i < num_polys; i++) {
                const u32 mtl_idx{ m.material_indices[i] };
                if (mtl_idx != material_idx) continue;

                const u32 index{ i * 3 };
                for (u32 j = index; j < index + 3; j++) {
                    const u32 v_idx{ m.raw_indices[j] };
                    if (vertex_ref[v_idx] != u32_invalid_id)
                        submesh.raw_indices.emplace_back(vertex_ref[v_idx]);
                    else {
                        submesh.raw_indices.emplace_back((u32)submesh.positions.size());
                        vertex_ref[v_idx] = submesh.raw_indices.back();
                        submesh.positions.emplace_back(m.positions[v_idx]);
                    }

                    if (m.normals.size()) submesh.normals.emplace_back(m.normals[j]);
                    if (m.tangents.size()) submesh.tangents.emplace_back(m.tangents[j]);

                    for (u32 k{ 0 }; k < m.uv_sets.size(); k++)
                        if (m.uv_sets[k].size())
                            submesh.uv_sets[k].emplace_back(m.uv_sets[k][j]);
                }
            }

            assert((submesh.raw_indices.size() % 3) == 0);
            return !submesh.raw_indices.empty();
        }

        void SplitMeshesByMaterial(Scene& scene, Progression* const progression) {
            assert(progression);
            progression->Callback(0, 0);
            for (auto& lod : scene.lod_groups) {
                util::vector<Mesh> new_meshes;
                for (auto& m : lod.meshes) {
                    const u32 num_materials{ (u32)m.material_used.size() };
                    if (num_materials > 1) {
                        for (u32 i{ 0 }; i < num_materials; i++) {
                            Mesh submesh{};
                            if (SplitMeshesByMaterial(m.material_used[i], m, submesh)) {
                                new_meshes.emplace_back(submesh);
                            }
                        }
                    }
                    else {
                        new_meshes.emplace_back(m);
                    }
                }
                progression->Callback(progression->Value(), progression->Maximum() + (u32)new_meshes.size());
                new_meshes.swap(lod.meshes);
            }
        }

        template<typename T>
        void AppendToVectorPOD(util::vector<T>& dst, const util::vector<T>& src) {
            if (src.empty()) return;
            const u32 num_elements{ (u32)dst.size() };
            dst.resize(dst.size() + src.size());
            memcpy(&dst[num_elements], src.data(), src.size() * sizeof(T));
        }

    } // anonymous namespace

    void ProcessScene(Scene& scene, const GeometryImportSettings& settings, Progression* const progression)
    {
        SplitMeshesByMaterial(scene, progression);

        for (auto& lod : scene.lod_groups)
            for (auto& m : lod.meshes) {
                ProcessVertices(m, settings);
                progression->Callback(progression->Value() + 1, progression->Maximum());
            }
    }

    void PackData(const Scene& scene, SceneData& data)
    {
        const u64 scene_size{ GetSceneSize(scene) };
        data.size = (u32)scene_size;
        data.data = (u8*)CoTaskMemAlloc(scene_size);
        assert(data.data);

        util::BlobStreamWriter blob{ data.data, data.size };

        // scene name
        blob.write((u32)scene.name.size());
        blob.write(scene.name.c_str(), scene.name.size());
        // number of LODs
        blob.write((u32)scene.lod_groups.size());

        for (auto& lod : scene.lod_groups)
        {
            // LOD name
            blob.write((u32)lod.name.size());
            blob.write(lod.name.c_str(), lod.name.size());
            // number of Meshes in this LOD
            blob.write((u32)lod.meshes.size());

            for (auto& m : lod.meshes)
                PackMeshData(m, blob);
        }

        assert(scene_size == blob.Offset());
    }

    bool CoalesceMeshes(const LODGroup& lod, Mesh& combined_mesh, Progression* const progression) {
        assert(lod.meshes.size());
        const Mesh& first_mesh{ lod.meshes[0] };
        combined_mesh.name = first_mesh.name;
        combined_mesh.elements_type = DetermineElementType(first_mesh);
        combined_mesh.lod_threshold = first_mesh.lod_threshold;
        combined_mesh.lod_id = first_mesh.lod_id;
        combined_mesh.uv_sets.resize(first_mesh.uv_sets.size());

        for (u32 mesh_idx{ 0 }; mesh_idx < lod.meshes.size(); mesh_idx++) {
            const Mesh& m{ lod.meshes[mesh_idx] };

            if (combined_mesh.elements_type != DetermineElementType(m) ||
                combined_mesh.uv_sets.size() != m.uv_sets.size() ||
                combined_mesh.lod_id != m.lod_id ||
                !Math::IsEqual(combined_mesh.lod_threshold, m.lod_threshold)) {
                combined_mesh = {};
                return false;
            }
        }

        for (u32 mesh_idx{ 0 }; mesh_idx < lod.meshes.size(); mesh_idx++) {
            const Mesh& m{ lod.meshes[mesh_idx] };

            if (combined_mesh.elements_type != DetermineElementType(m) ||
                combined_mesh.uv_sets.size() != m.uv_sets.size() ||
                combined_mesh.lod_id != m.lod_id ||
                !Math::IsEqual(combined_mesh.lod_threshold, m.lod_threshold)) {
                combined_mesh = {};
                return false;
            }

            const u32 position_count{ (u32)combined_mesh.positions.size() };
            const u32 raw_index_base{ (u32)combined_mesh.raw_indices.size() };

            AppendToVectorPOD(combined_mesh.positions, m.positions);
            AppendToVectorPOD(combined_mesh.normals, m.normals);
            AppendToVectorPOD(combined_mesh.tangents, m.tangents);
            AppendToVectorPOD(combined_mesh.colors, m.colors);

            for (u32 i{ 0 }; i < combined_mesh.uv_sets.size(); i++) {
                AppendToVectorPOD(combined_mesh.uv_sets[i], m.uv_sets[i]);
            }

            AppendToVectorPOD(combined_mesh.material_indices, m.material_indices);
            AppendToVectorPOD(combined_mesh.raw_indices, m.raw_indices);

            for (u32 i{ raw_index_base }; i < combined_mesh.raw_indices.size(); i++) {
                combined_mesh.raw_indices[i] += position_count;
            }

            progression->Callback(progression->Value(), progression->Maximum() > 0 ? progression->Maximum() - 1 : 1);
        }

        for (const u32 mtl_idx : combined_mesh.material_indices) {
            if (std::find(combined_mesh.material_used.begin(), combined_mesh.material_used.end(), mtl_idx) == combined_mesh.material_used.end()) {
                combined_mesh.material_used.emplace_back(mtl_idx);
            }
        }

        return true;
    }
}