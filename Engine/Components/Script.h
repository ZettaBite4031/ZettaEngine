#pragma once
#include "ComponentCommon.h"

namespace Zetta::Script {

	struct InitInfo {
		Detail::ScriptCreator script_creator;
	};

	Component CreateScript(const InitInfo& info, GameEntity::Entity EntityID);
	void RemoveScript(Component c);
	void Update(float dt);
	
}
