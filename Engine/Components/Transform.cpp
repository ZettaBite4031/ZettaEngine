#include "Transform.h"
#include "Entity.h"

namespace Zetta::Transform {
	namespace {
		util::vector<Math::v3> positions;
		util::vector<Math::v3> orientations;
		util::vector<Math::v4> rotations;
		util::vector<Math::v3> scales;

		Math::v3 CalculateOrientation(Math::v4 rotation) {
			using namespace DirectX;
			XMVECTOR rotation_quat{ XMLoadFloat4(&rotation) };
			XMVECTOR front{ XMVectorSet(0.f, 0.f, 1.f, 0.f) };
			Math::v3 orientation;
			XMStoreFloat3(&orientation, XMVector3Rotate(front, rotation_quat));
			return orientation;
		}
	}

	Component CreateTransform(const InitInfo& info, GameEntity::Entity entity) {
		assert(entity.IsValid());
		const ID::ID_Type entity_id{ ID::Index(entity.GetID()) };

		if (positions.size() > entity_id) {
			Math::v4 rotation{ info.rotation };
			rotations[entity_id] = rotation;
			orientations[entity_id] = CalculateOrientation(rotation);
			positions[entity_id] = Math::v3{ info.position };
			scales[entity_id] = Math::v3{ info.scale };
		}
		else {
			assert(positions.size() == entity_id);
			Math::v4 rotation{ info.rotation };
			rotations.emplace_back(rotation);
			orientations.emplace_back(CalculateOrientation(rotation));
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
		}

		return Component{ TransformID{ entity.GetID() } };
	}

	void RemoveTransform([[maybe_unused]] Component c) {
		assert(c.IsValid());

	}

	Math::v4 Component::Rotation() const {
		assert(IsValid());
		return rotations[ID::Index(_id)];
	}

	Math::v3 Component::Orientation() const {
		assert(IsValid());
		return orientations[ID::Index(_id)];
	}

	Math::v3 Component::Position() const {
		assert(IsValid());
		return positions[ID::Index(_id)];
	}
	
	Math::v3 Component::Scale() const {
		assert(IsValid());
		return scales[ID::Index(_id)];
	}
}