#pragma once

#include "ComponentCommon.h"

namespace Zetta {
#define INIT_INFO(component) namespace component { struct InitInfo; }
	INIT_INFO(Transform);
	INIT_INFO(Script);
#undef INIT_INFO

	namespace GameEntity {
		struct EntityInfo {
			Transform::InitInfo* transform{ nullptr };
			Script::InitInfo* script{ nullptr };
		};

		Entity CreateGameEntity(const EntityInfo& info);
		void RemoveGameEntity(EntityID id);
		bool IsAlive(EntityID id);
	}
} 