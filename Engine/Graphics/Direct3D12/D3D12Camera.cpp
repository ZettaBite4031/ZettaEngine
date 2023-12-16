#include "D3D12Camera.h"
#include "EngineAPI/GameEntity.h"

namespace Zetta::Graphics::D3D12::Camera {
	namespace {
		util::FreeList<D3D12Camera> cameras;

#pragma region Getters & Setters
		void SetUpVector(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			Math::v3 up{ *(Math::v3*)data };
			assert(sizeof(up) == size);
			camera.up(up);
		}

		constexpr void SetFOV(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Perspective);
			f32 fov{ *(f32*)data };
			assert(sizeof(fov) == size);
			camera.FOV(fov);
		}

		constexpr void SetAspectRatio(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Perspective);
			f32 aspect_ratio{ *(f32*)data };
			assert(sizeof(aspect_ratio) == size);
			camera.AspectRatio(aspect_ratio);
		}

		constexpr void SetViewWidth(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Orthographic);
			f32 view_width{ *(f32*)data };
			assert(sizeof(view_width) == size);
			camera.ViewWidth(view_width);
		}

		constexpr void SetViewHeight(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Orthographic);
			f32 view_height{ *(f32*)data };
			assert(sizeof(view_height) == size);
			camera.ViewHeight(view_height);
		}

		constexpr void SetNearZ(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			f32 near_z{ *(f32*)data };
			assert(sizeof(near_z) == size);
			camera.NearZ(near_z);
		}

		constexpr void SetFarZ(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			f32 far_z{ *(f32*)data };
			assert(sizeof(far_z) == size);
			camera.FarZ(far_z);
		}

		void GetView(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::mat4* const matrix{ (Math::mat4* const)data };
			assert(sizeof(matrix) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.ViewProjection());
		}

		void GetViewProjection(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::mat4* const matrix{ (Math::mat4* const)data };
			assert(sizeof(matrix) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.View());
		}

		void GetProjection(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::mat4* const matrix{ (Math::mat4* const)data };
			assert(sizeof(Math::mat4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.Projection());
		}

		void GetInverseView(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::mat4* const matrix{ (Math::mat4* const)data };
			assert(sizeof(Math::mat4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.InverseViewProjection());
		}

		void GetInverseProjection(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::mat4* const matrix{ (Math::mat4* const)data };
			assert(sizeof(Math::mat4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.InverseProjection());
		}

		void GetUpVector(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Math::v3 *const up{ (Math::v3* const)data };
			assert(sizeof(Math::v3) == size);
			DirectX::XMStoreFloat3(up, camera.up());
		}

		constexpr void GetFOV(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Perspective);
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.FOV();
		}

		constexpr void GetAspectRatio(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Perspective);
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.AspectRatio();
		}

		constexpr void GetViewWidth(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Orthographic);
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.ViewWidth();
		}

		constexpr void GetViewHeight(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.ProjectionType() == Graphics::Camera::Orthographic);
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.ViewHeight();
		}

		constexpr void GetNearZ(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.NearZ();
		}

		constexpr void GetFarZ(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			f32* const var{ (f32*)data };
			assert(sizeof(f32) == size);
			*var = camera.FarZ();
		}

		constexpr void GetProjectionType(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			Graphics::Camera::Type* const type{ (Graphics::Camera::Type* const)data };
			assert(sizeof(Graphics::Camera::Type) == size);
			*type = camera.ProjectionType();
		}

		constexpr void GetEntityID(D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			ID::ID_Type* const id{ (ID::ID_Type* const)data };
			assert(sizeof(ID::ID_Type) == size);
			*id = camera.EntityID();
		}

		void InvalidSet(D3D12Camera&, const void* const, u32) {}

		using SetFunction = void(*)(D3D12Camera&, const void* const, u32);
		using GetFunction = void(*)(D3D12Camera&, void* const, u32);

		constexpr SetFunction SetFunctions[]{
			SetUpVector,
			SetFOV,
			SetAspectRatio,
			SetViewWidth,
			SetViewHeight,
			SetNearZ,
			SetFarZ,
			InvalidSet,
			InvalidSet,
			InvalidSet,
			InvalidSet,
			InvalidSet,
			InvalidSet,
			InvalidSet
		};

		static_assert(_countof(SetFunctions) == Graphics::CameraParameter::count);

		constexpr GetFunction GetFunctions[]{
			GetUpVector,
			GetFOV,
			GetAspectRatio,
			GetViewWidth,
			GetViewHeight,
			GetNearZ,
			GetFarZ,
			GetView,
			GetProjection,
			GetInverseProjection,
			GetViewProjection,
			GetInverseView,
			GetProjectionType,
			GetEntityID
		};

		static_assert(_countof(GetFunctions) == Graphics::CameraParameter::count);
#pragma endregion

	}

	D3D12Camera::D3D12Camera(CameraInitInfo info) 
		: _up{ DirectX::XMLoadFloat3(&info.up) },
		_near_z{ info.near_z }, _far_z{ info.far_z },
		_fov{ info.fov }, _aspect_ratio{ info.aspect_ratio },
		_projection_type{ info.type }, _entity_id{ info.entity_id }, _is_dirty{ true } {
		assert(ID::IsValid(_entity_id));
		Update();
	}

	void D3D12Camera::Update() {
		GameEntity::Entity entity{ GameEntity::EntityID{_entity_id} };
		using namespace DirectX;
		Math::v3 pos{ entity.Transform().Position() };
		Math::v3 dir{ entity.Transform().Orientation() };
		_position = XMLoadFloat3(&pos);
		_direction = XMLoadFloat3(&dir);
		_view = XMMatrixLookToRH(_position, _direction, _up);

		if (_is_dirty) {
			// NOTE: the NearZ and FarZ values are swapped because the renderer uses inverse depth.
			_projection = (_projection_type == Graphics::Camera::Perspective) ?
				XMMatrixPerspectiveFovRH(_fov * XM_PI, _aspect_ratio, _far_z, _near_z) :
				XMMatrixOrthographicRH(_view_width, _view_height, _far_z, _near_z);
			_inverse_projection = XMMatrixInverse(nullptr, _projection);
			_is_dirty = false;
		}

		_view_projection = XMMatrixMultiply(_view, _projection);
		_inverse_view_projection = XMMatrixInverse(nullptr, _view_projection);
	}

	void D3D12Camera::up(Math::v3 up) {
		_up = DirectX::XMLoadFloat3(&up);
	}

	constexpr void D3D12Camera::FOV(f32 fov) {
		assert(_projection_type == Graphics::Camera::Perspective);
		_fov = fov;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::AspectRatio(f32 aspect_ratio) {
		assert(_projection_type == Graphics::Camera::Perspective);
		_aspect_ratio = aspect_ratio;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::ViewWidth(f32 width) {
		assert(width);
		assert(_projection_type == Graphics::Camera::Orthographic);
		_view_width = width;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::ViewHeight(f32 height) {
		assert(height);
		assert(_projection_type == Graphics::Camera::Orthographic);
		_view_height = height;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::NearZ(f32 nearZ) {
		_near_z = nearZ;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::FarZ(f32 farZ) {
		_far_z = farZ;
		_is_dirty = true;
	}

	Graphics::Camera Create(CameraInitInfo info) {
		return Graphics::Camera{ CameraID{cameras.Add(info)} };
	}

	void Remove(CameraID id) {
		assert(ID::IsValid(id));
		cameras.Remove(id);
	}

	void SetParameter(CameraID id, CameraParameter::Parameter param, const void* const data, u32 size) {
		assert(data && size);
		assert(param < CameraParameter::count);
		D3D12Camera& camera{ Get(id) };
		SetFunctions[param](camera, data, size);
	}

	void GetParameter(CameraID id, CameraParameter::Parameter param, void* const data, u32 size) {
		assert(data && size);
		assert(param < CameraParameter::count);
		D3D12Camera& camera{ Get(id) };
		GetFunctions[param](camera, data, size);
	}

	[[nodiscard]] D3D12Camera& Get(CameraID id) {
		assert(ID::IsValid(id));
		return cameras[id];
	}

}