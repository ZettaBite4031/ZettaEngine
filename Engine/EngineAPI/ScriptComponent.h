#pragma once
#include "Components/ComponentCommon.h"

namespace Zetta::Script {
	DEFINE_TYPED_ID(ScriptID);
	class Component final {
	public:
		constexpr explicit Component(ScriptID id) : _id{ id } {}
		constexpr Component() : _id{ ID::Invalid_ID } {}
		constexpr ScriptID GetID() const { return _id; }
		constexpr bool IsValid() const { return ID::IsValid(_id); }

		Math::v3 Position() const;
		Math::v4 Rotation() const;
		Math::v3 Scale() const;

	private:
		ScriptID _id;
	};
}