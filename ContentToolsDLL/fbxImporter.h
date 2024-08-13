#pragma once

#include "ToolsCommon.h"

#include <fbxsdk.h>

namespace Zetta::Tools {

	struct SceneData;
	struct Scene;
	struct Mesh;
	struct GeometryImportSettings;

	class FBXContext {
	public:
		FBXContext(const char* file, Scene* scene, SceneData* data, Progression* const progression)
			: _scene{ scene }, _scene_data{ data }, _progression{ progression } {
			assert(file && _scene && _scene_data);
			if (InitializeFBX()) LoadFBXFile(file);
		}

		~FBXContext() {
			_fbx_scene->Destroy();
			_fbx_manager->Destroy();
			ZeroMemory(this, sizeof(FBXContext));
		}

		void GetScene(FbxNode* node = nullptr);

		constexpr bool IsValid() const { return _fbx_manager && _fbx_scene; }
		constexpr f32 SceneScale() const { return _scene_scale; }
		constexpr Progression* GetProgression() const { return _progression; }

	private:
		bool InitializeFBX();
		void LoadFBXFile(const char* file);
		void GetMeshes(FbxNode* node, util::vector<Mesh>& meshes, u32 lod_id, f32 lod_threshold);
		void GetMesh(FbxNodeAttribute* attribute, util::vector<Mesh>& meshes, u32 lod_id, f32 lod_threshold);
		void GetLODGroup(FbxNodeAttribute* attribute);
		bool GetMeshData(FbxMesh* fbx_mesh, Mesh& m);

		Scene* _scene{ nullptr };
		SceneData* _scene_data{ nullptr };
		FbxManager* _fbx_manager{ nullptr };
		FbxScene* _fbx_scene{ nullptr };
		Progression* _progression{ nullptr };
		f32 _scene_scale{ 1.0f };
	};
}