#include "assimpImporter.h"
#include "Geometry.h"

#include <mutex>

namespace Zetta::Tools {
	namespace {
		std::mutex assimp_mutex{};

		Mesh ASSIMPProcessMesh(aiMesh* ai_mesh, const aiScene* scene, const GeometryImportSettings& settings) {
			Mesh mesh{};
			mesh.uv_sets.push_back({});

			for (u32 i = 0; i < ai_mesh->mNumVertices; i++) {
				Math::v3 pos;
				pos.x = ai_mesh->mVertices[i].x;
				pos.y = ai_mesh->mVertices[i].y;
				pos.z = ai_mesh->mVertices[i].z;
				mesh.positions.push_back(pos);

				if (!settings.calculate_normals) {
					Math::v3 normal;
					normal.x = ai_mesh->mNormals[i].x;
					normal.y = ai_mesh->mNormals[i].y;
					normal.z = ai_mesh->mNormals[i].z;
					mesh.normals.push_back(normal);
				}

				if (ai_mesh->mTextureCoords[0]) {
					Math::v2 uv;
					uv.x = ai_mesh->mTextureCoords[0][i].x;
					uv.y = ai_mesh->mTextureCoords[0][i].y;
					mesh.uv_sets[0].push_back(uv);
				}

				if (!settings.calculate_tangents) {
					Math::v4 tangent;
					tangent.x = ai_mesh->mTangents[i].x;
					tangent.y = ai_mesh->mTangents[i].y;
					tangent.z = ai_mesh->mTangents[i].z;
					tangent.w = 0.f;
					mesh.tangents.push_back(tangent);
				}
			}
			
			for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
				const aiFace& face = ai_mesh->mFaces[i];
				for (u32 j = 0; j < face.mNumIndices; j++) {
					mesh.raw_indices.push_back(face.mIndices[j]);
				}
			}

			mesh.name = ai_mesh->mName.C_Str();
			return mesh;
		}

		void ASSIMPProcessScene(const aiScene* ai_scene, aiNode* node, Scene* scene, const GeometryImportSettings& settings) {
			for (u32 i = 0; i < node->mNumMeshes; i++) {
				aiMesh* mesh = ai_scene->mMeshes[node->mMeshes[i]];
				Mesh newMesh = ASSIMPProcessMesh(mesh, ai_scene, settings);
				scene->lod_groups[0].meshes.push_back(newMesh);
			}

			for (u32 i = 0; i < node->mNumChildren; i++) {
				ASSIMPProcessScene(ai_scene, node->mChildren[i], scene, settings);
			}
		}

		bool ASSIMPGetScene(const char* file, Scene* scene, SceneData* data) {
			Assimp::Importer importer;
			const aiScene* ai_scene = importer.ReadFile(file,
				aiProcess_CalcTangentSpace | aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

			if (ai_scene == nullptr || ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !ai_scene->mRootNode) return false;

			scene->name = ai_scene->mName.C_Str();

			LODGroup ASSIMP_import_LOD_group{};
			ASSIMP_import_LOD_group.name = "ASSIMP_Lod_Group";
			scene->lod_groups.push_back(ASSIMP_import_LOD_group);

			ASSIMPProcessScene(ai_scene, ai_scene->mRootNode, scene, data->settings);

			return true;
		}
	}

	EDITOR_INTERFACE void ImportASSIMP(const char* file, SceneData* data) {
		assert(file && data);
		Scene scene{};

		{
			std::lock_guard lock{ assimp_mutex };
			if (!ASSIMPGetScene(file, &scene, data)) return;
		}

		//ProcessScene(scene, data->settings);
		PackData(scene, *data);
		/*
			- Issue currently: Scene tree traversal is fucked somehow. Brain hurt. Will try to fix later
		*/
	}
}