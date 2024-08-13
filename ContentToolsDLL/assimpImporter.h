#pragma once

#include "ToolsCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Zetta::Tools {
	struct SceneData;
	struct Scene;
	struct Mesh;
	struct GeometryImportSettings;

	EDITOR_INTERFACE void ImportASSIMP(const char* file, SceneData* data);
}