#include "Common.h"
#include "../Common/CommonHeaders.h"
#include "../Common/ID.h"
#include "../Engine/Components/Entity.h"
#include "../Engine/Components/Transform.h"
#include "../Engine/Components/Script.h"

using namespace Zetta;

namespace {
	struct TransformComponent {
		f32 position[3];
		f32 rotation[3];
		f32 scale[3];

		void ToInitInfo(Transform::InitInfo& info) {
			using namespace DirectX;
			memcpy(&info.position[0], &position[0], sizeof(position));
			memcpy(&info.scale[0], &scale[0], sizeof(scale));
			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);
			memcpy(&info.rotation[0], &rot_quat.x, sizeof(info.rotation));
		}
	};

	struct ScriptComponent {
		Script::Detail::ScriptCreator script_creator;

		void ToInitInfo(Script::InitInfo& info) {
			info.script_creator = script_creator;
		}
	};

	struct GameEntityDescriptor {
		TransformComponent transform;
		ScriptComponent script;
	};
}

EDITOR_INTERFACE ID::ID_Type CreateGameEntity(GameEntityDescriptor* e) {
	assert(e);
	GameEntityDescriptor& desc{ *e };
	Transform::InitInfo Transform_Info;
	Script::InitInfo Script_Info;
	desc.transform.ToInitInfo(Transform_Info);
	desc.script.ToInitInfo(Script_Info);
	GameEntity::EntityInfo Entity_Info{ &Transform_Info, &Script_Info };
	return GameEntity::CreateGameEntity(Entity_Info).GetID();
}

EDITOR_INTERFACE void RemoveGameEntity(ID::ID_Type id) {
	assert(ID::IsValid(id));
	GameEntity::RemoveGameEntity(GameEntity::EntityID{ id });
}