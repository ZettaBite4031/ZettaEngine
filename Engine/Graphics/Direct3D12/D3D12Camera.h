#pragma once 
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Camera {
	class D3D12Camera {
	public:
		explicit D3D12Camera(CameraInitInfo info);

		void Update();

		void up(Math::v3 up);
		constexpr void FOV(f32 fov);
		constexpr void AspectRatio(f32 aspect_ratio);
		constexpr void ViewWidth(f32 width);
		constexpr void ViewHeight(f32 height);
		constexpr void NearZ(f32 nearZ);
		constexpr void FarZ(f32 farZ);

		[[nodiscard]] constexpr DirectX::XMMATRIX View() const { return _view; }
		[[nodiscard]] constexpr DirectX::XMMATRIX Projection() const { return _projection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX InverseProjection() const { return _inverse_projection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX ViewProjection() const { return _view_projection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX InverseViewProjection() const { return _inverse_view_projection; }
		[[nodiscard]] constexpr DirectX::XMVECTOR up() const { return _up; }
		[[nodiscard]] constexpr DirectX::XMVECTOR Position() const { return _position; }
		[[nodiscard]] constexpr DirectX::XMVECTOR Direction() const { return _direction; }
		[[nodiscard]] constexpr f32 NearZ() const { return _near_z; }
		[[nodiscard]] constexpr f32 FarZ() const { return _far_z; }
		[[nodiscard]] constexpr f32 FOV() const { return _fov; }
		[[nodiscard]] constexpr f32 AspectRatio() const { return _aspect_ratio; }
		[[nodiscard]] constexpr f32 ViewWidth() const { return _view_width; }
		[[nodiscard]] constexpr f32 ViewHeight() const { return _view_height; }
		[[nodiscard]] constexpr Graphics::Camera::Type ProjectionType() const { return _projection_type; }
		[[nodiscard]] constexpr ID::ID_Type EntityID() const { return _entity_id; }

	private:
		DirectX::XMMATRIX			_view;
		DirectX::XMMATRIX			_projection;
		DirectX::XMMATRIX			_inverse_projection;
		DirectX::XMMATRIX			_view_projection;
		DirectX::XMMATRIX			_inverse_view_projection;
		DirectX::XMVECTOR			_position{};
		DirectX::XMVECTOR			_direction{};
		DirectX::XMVECTOR			_up;
		f32							_near_z;
		f32							_far_z;
		union {
			f32						_fov;
			f32						_view_width;
		};
		union {
			f32						_aspect_ratio;
			f32						_view_height;
		};
		Graphics::Camera::Type		_projection_type;
		ID::ID_Type					_entity_id;
		bool						_is_dirty;
	};

	Graphics::Camera Create(CameraInitInfo info);
	void Remove(CameraID id);
	void SetParameter(CameraID id, CameraParameter::Parameter param, const void* const data, u32 size);
	void GetParameter(CameraID id, CameraParameter::Parameter param, void* const data, u32 size);
	[[nodiscard]] D3D12Camera& Get(CameraID id);
}
