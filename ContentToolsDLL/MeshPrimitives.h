#pragma once
#include "ToolsCommon.h"

namespace Zetta::Tools {
	enum PrimitiveMeshType : u32 {
		Plane,
		Cube,
		UVSphere,
		Icosphere,
		Cylinder,
		Capsule,

		count
	};

	struct PrimitiveInitInfo {
		PrimitiveMeshType	type;
		u32					segments[3]{ 1,1,1 };
		Math::v3			size{ 1,1,1 };
		u32					lod{ 0 };
	};
}