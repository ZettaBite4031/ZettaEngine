#pragma once
#include "CommonHeaders.h"

namespace Zetta::Graphics {
	
	DEFINE_TYPED_ID(CameraID);

	class Camera {
	public:
		enum Type : u32 {
			Perspective,
			Orthographic
		};

		constexpr explicit Camera(CameraID id) : _id{ id } {}
		constexpr Camera() = default;
		constexpr CameraID GetID() const { return _id; }
		constexpr bool IsValid() const { return ID::IsValid(_id); }

		void up(Math::v3 up) const;
		void FOV(f32 fov) const;
		void AspectRatio(f32 aspect_ratio) const;
		void ViewWidth(f32 width) const;
		void ViewHeight(f32 height) const;
		void Range(f32 nearZ, f32 farZ) const;

		Math::mat4 View() const;
		Math::mat4 Projection() const;
		Math::mat4 InverseProjection() const;
		Math::mat4 ViewProjection() const;
		Math::mat4 InverseViewProjection() const;
		
		Math::v3 up() const;
		f32 NearZ() const;
		f32 FarZ() const;
		f32 FOV() const;
		f32 AspectRatio() const;
		f32 ViewWidth() const;
		f32 ViewHeight() const;

		Type ProjectionType() const;

		ID::ID_Type EntityID() const;

	private:
		CameraID _id{ ID::Invalid_ID };
	};
}