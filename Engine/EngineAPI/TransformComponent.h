#pragma once
#include "Components/ComponentCommon.h"

namespace Zetta::Transform {
	DEFINE_TYPED_ID(TransformID);
	class Component final {
	public:
		constexpr explicit Component(TransformID id) : _id{ id } {}
		constexpr Component() : _id{ ID::Invalid_ID } {}
		constexpr TransformID GetID() const { return _id; }
		constexpr bool IsValid() const { return ID::IsValid(_id); }

		Math::v4 Rotation() const;
		Math::v3 Orientation() const;
		Math::v3 Position() const;
		Math::v3 Scale() const;

	private:
		TransformID _id;
	};
}