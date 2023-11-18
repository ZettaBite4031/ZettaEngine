#include "Transform.h"
#include "Entity.h"

namespace Zetta::Transform {
	namespace {
		util::vector<Math::v3> positions;
		util::vector<Math::v4> rotations;
		util::vector<Math::v3> scales;
	}

	Component CreateTransform(const InitInfo& info, GameEntity::Entity entity) {
		assert(entity.IsValid());
		const ID::ID_Type entity_id{ ID::Index(entity.GetID()) };

		if (positions.size() > entity_id) {
			positions[entity_id] = Math::v3(info.position);
			rotations[entity_id] = Math::v4(info.rotation);
			scales[entity_id] = Math::v3(info.scale);
		}
		else {
			assert(positions.size() == entity_id);
			positions.emplace_back(info.position);
			rotations.emplace_back(info.rotation);
			scales.emplace_back(info.scale);
		}

		return Component{ TransformID{ entity.GetID() } };
	}

	void RemoveTransform([[maybe_unused]] Component c) {
		assert(c.IsValid());

	}

	Math::v3 Component::Position() const {
		assert(IsValid());
		return positions[ID::Index(_id)];
	}

	Math::v4 Component::Rotation() const {
		assert(IsValid());
		return rotations[ID::Index(_id)];
	}

	Math::v3 Component::Scale() const {
		assert(IsValid());
		return scales[ID::Index(_id)];
	}
}