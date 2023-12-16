#include "Transform.h"
#include "Entity.h"

namespace Zetta::Transform {
	namespace {
		util::vector<Math::mat4>	to_world;
		util::vector<Math::mat4>	inv_world;
		util::vector<Math::v3>		positions;
		util::vector<Math::v3>		orientations;
		util::vector<Math::v4>		rotations;
		util::vector<Math::v3>		scales;
		util::vector<u8>			has_transform;
		util::vector<u8>			changes_from_previous_frame;
		u8							read_write_flag;

		void CalculateTransformMatrices(ID::ID_Type index) {
			assert(rotations.size() >= index);
			assert(positions.size() >= index);
			assert(scales.size() >= index);

			using namespace DirectX;
			XMVECTOR r{ XMLoadFloat4(&rotations[index]) };
			XMVECTOR t{ XMLoadFloat3(&positions[index]) };
			XMVECTOR s{ XMLoadFloat3(&scales[index]) };

			XMMATRIX world{ XMMatrixAffineTransformation(s, XMQuaternionIdentity(), r, t) };
			XMStoreFloat4x4(&to_world[index], world);

			world.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
			XMMATRIX inverse_world{ XMMatrixInverse(nullptr, world) };
			XMStoreFloat4x4(&inv_world[index], inverse_world);

			has_transform[index] = 1;
		}

		Math::v3 CalculateOrientation(Math::v4 rotation) {
			using namespace DirectX;
			XMVECTOR rotation_quat{ XMLoadFloat4(&rotation) };
			XMVECTOR front{ XMVectorSet(0.f, 0.f, 1.f, 0.f) };
			Math::v3 orientation;
			XMStoreFloat3(&orientation, XMVector3Rotate(front, rotation_quat));
			return orientation;
		}

		void SetRotation(TransformID id, const Math::v4& rotation_quaternion) {
			const u32 index{ ID::Index(id) };
			rotations[index] = rotation_quaternion;
			orientations[index] = CalculateOrientation(rotation_quaternion);
			has_transform[index] = 0;
			changes_from_previous_frame[index] |= ComponentFlags::Rotation;
		}

		void SetOrientation(TransformID, const Math::v3&) {

		}

		void SetPosition(TransformID id, const Math::v3& position) {
			const u32 index{ ID::Index(id) };
			positions[index] = position;
			has_transform[index] = 0;
			changes_from_previous_frame[index] |= ComponentFlags::Position;
		}

		void SetScale(TransformID id, const Math::v3& scale) {
			const u32 index{ ID::Index(id) };
			scales[index] = scale;
			has_transform[index] = 0;
			changes_from_previous_frame[index] |= ComponentFlags::Scale;
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
			has_transform[entity_id] = 0;
			changes_from_previous_frame[entity_id] = (u8)ComponentFlags::All;
		}
		else {
			assert(positions.size() == entity_id);
			Math::v4 rotation{ info.rotation };
			to_world.emplace_back();
			inv_world.emplace_back();
			rotations.emplace_back(rotation);
			orientations.emplace_back(CalculateOrientation(rotation));
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
			has_transform.emplace_back((u8)0);
			changes_from_previous_frame.emplace_back((u8)ComponentFlags::All);
		}

		return Component{ TransformID{ entity.GetID() } };
	}

	void RemoveTransform([[maybe_unused]] Component c) {
		assert(c.IsValid());

	}

	void GetTransformMatrices(const GameEntity::EntityID id, Math::mat4& world, Math::mat4& inverse_world) {
		assert(GameEntity::Entity{ id }.IsValid());

		const ID::ID_Type entity_index{ ID::Index(id) };
		if (!has_transform[entity_index]) CalculateTransformMatrices(entity_index);

		world = to_world[entity_index];
		inverse_world = inv_world[entity_index];
	}

	void GetUpdatedComponentFlags(const GameEntity::EntityID* const ids, u32 count, u8* const flags) {
		assert(ids && count && flags);
		read_write_flag = 1;

		for (u32 i{ 0 }; i < count; i++) {
			assert(GameEntity::Entity{ ids[i] }.IsValid());
			flags[i] = changes_from_previous_frame[ID::Index(ids[i])];
		}
	}

	void Update(const ComponentCache* const cache, u32 count) {
		assert(cache && count);
		if (read_write_flag) {
			memset(changes_from_previous_frame.data(), 0, changes_from_previous_frame.size());
			read_write_flag = 0;
		}

		for (u32 i{ 0 }; i < count; i++) {
			const ComponentCache& c{ cache[i] };
			assert(Component{ c.id }.IsValid());

			if (c.flags & ComponentFlags::Rotation) SetRotation(c.id, c.rotation);
			if (c.flags & ComponentFlags::Orientation) SetOrientation(c.id, c.orientation);
			if (c.flags & ComponentFlags::Position) SetPosition(c.id, c.position);
			if (c.flags & ComponentFlags::Scale) SetScale(c.id, c.scale);
						
		}
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