#pragma once
#include "ComponentCommon.h"
	
namespace Zetta::Transform {

	struct InitInfo {
		f32 position[3]{};
		f32 rotation[4]{};
		f32 scale[3]{ 1.f, 1.f, 1.f };
	};

	struct ComponentFlags {
		enum Flags : u32 {
			Rotation = 0x01,
			Orientation = 0x02,
			Position = 0x04,
			Scale = 0x08,

			All = Rotation | Orientation | Position | Scale
		};
	};

	struct ComponentCache {
		Math::v4 rotation;
		Math::v3 orientation;
		Math::v3 position;
		Math::v3 scale;
		TransformID id;
		u32 flags;
	};

	Component CreateTransform(const InitInfo& info, GameEntity::Entity EntityID);
	void RemoveTransform(Component c);
	void GetTransformMatrices(const GameEntity::EntityID id, Math::mat4& world, Math::mat4& inverse_world);
	void GetUpdatedComponentFlags(const GameEntity::EntityID* const id, u32 count, u8* const flags);
	void Update(const ComponentCache* const cache, u32 count);
}
