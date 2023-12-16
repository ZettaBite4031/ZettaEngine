#pragma once
#include "CommonHeaders.h"

namespace Zetta::Graphics {

	DEFINE_TYPED_ID(LightID);

	class Light {
	public:
		enum Type : u32 {
			Directional,
			Point,
			Spot,
			
			count
		};

		constexpr explicit Light(LightID id, u64 light_set_key) : _light_set_key{ light_set_key }, _id { id } {}
		constexpr Light() = default;
		constexpr LightID GetID() const { return _id; }
		constexpr u64 GetLightSetKey() const { return _light_set_key; }
		constexpr bool IsValid() const { return ID::IsValid(_id); }
		
		void IsEnabled(bool is_enabled) const;
		void Intensity(f32 intensity) const;
		void Color(Math::v3 color) const;
		void Attenuation(Math::v3 attenuation) const;
		void Range(f32 range) const;
		void ConeAngles(f32 umbra, f32 penumbra) const;

		bool IsEnabled() const;
		f32 Intensity() const;
		Math::v3 Color() const;
		Math::v3 Attenuation() const;
		f32 Range() const;
		f32 Umbra() const;
		f32 Penumbra() const;

		Type LightType() const;

		ID::ID_Type EntityID() const;

	private:
		u64 _light_set_key{ 0 };
		LightID _id{ ID::Invalid_ID };
	};
}