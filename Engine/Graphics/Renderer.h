#pragma once
#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"

namespace Zetta::Graphics {
	DEFINE_TYPED_ID(SurfaceID);

	class Surface {
	public:
		constexpr explicit Surface(SurfaceID id) : _id{ id } {}
		constexpr Surface() = default;
		constexpr SurfaceID GetID() const { return _id; }
		const bool IsValid() const { return ID::IsValid(_id); }

		void Resize(u32, u32) const;
		const u32 Width() const;
		const u32 Height() const;
		void Render() const;

	private:
		SurfaceID _id{ ID::Invalid_ID };
	};

	struct RenderSurface {
		Platform::Window window{};
		Surface surface{};
	};

	struct CameraParameter {
		enum Parameter : u32 {
			UpVector,
			FOV,
			AspectRatio,
			ViewWidth,
			ViewHeight,
			NearZ, 
			FarZ, 
			View,
			Projection,
			InverseProjection,
			ViewProjection,
			InserveViewProjection,
			Type, 
			EntityID,
			
			count
		};
	};

	struct CameraInitInfo {
		ID::ID_Type entity_id{ ID::Invalid_ID };
		Camera::Type type{};
		Math::v3 up;
		union {
			f32 fov;
			f32 view_width;
		};
		union {
			f32 aspect_ratio;
			f32 view_height;
		};
		f32 near_z;
		f32 far_z;
	};

	struct PerspectiveCameraInitInfo : public CameraInitInfo {
		explicit PerspectiveCameraInitInfo(ID::ID_Type id) {
			assert(ID::IsValid(id));
			entity_id = id;
			type = Camera::Perspective;
			up = { 0.f, 1.f, 0.f };
			fov = 0.25f;
			aspect_ratio = 16.f / 9.f;
			near_z = 0.001f;
			far_z = 10000.f;
		}
	};

	struct OrthographicCameraInitInfo : public CameraInitInfo {
		explicit OrthographicCameraInitInfo(ID::ID_Type id) {
			assert(ID::IsValid(id));
			entity_id = id;
			type = Camera::Orthographic;
			up = { 0.f, 1.f, 0.f };
			view_width = 1920;
			view_height = 1080;
			near_z = 0.001f;
			far_z = 10000.f;
		}
	};

	struct PrimitiveTopology {
		enum Type : u32 {
			PointList = 1,
			LineList,
			LineStrip,
			TriangleList,
			TriangleStrip,

			count
		};
	};

	enum GraphicsPlatform {
		Direct3D12 = 0,
		Vulkan = 1, // UNIMPLEMENTED - DO NOT USE
		OpenGL = 2  // UNIMPLEMENTED - DO NOT USE
	};

	bool Initialize(GraphicsPlatform platform);
	void Shutdown();

	const char* GetEngineShadersPath();
	const char* GetEngineShadersPath(GraphicsPlatform);

	Surface CreateSurface(Platform::Window window);
	void RemoveSurface(SurfaceID id);

	Camera CreateCamera(CameraInitInfo info);
	void RemoveCamera(CameraID id);

	ID::ID_Type AddSubmesh(const u8*& data);
	void RemoveSubmesh(ID::ID_Type id);
}