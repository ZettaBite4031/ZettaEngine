#pragma once
#include "ComponentCommon.h"

namespace Zetta::Transform {

	struct InitInfo {
		f32 position[3]{};
		f32 rotation[4]{};
		f32 scale[3]{ 1.f, 1.f, 1.f };
	};

	Component CreateTransform(const InitInfo& info, GameEntity::Entity EntityID);
	void RemoveTransform(Component c);
}
