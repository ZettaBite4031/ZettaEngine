#pragma once
#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"
#include "EngineAPI/Light.h"

namespace Zetta::Graphics {
	DEFINE_TYPED_ID(SurfaceID);

	struct FrameInfo {
		ID::ID_Type* render_item_ids{ nullptr };
		f32* thresholds{ nullptr };
		u64 light_set_key;
		f32 last_frame_time{ 16.7f };
		f32 average_frame_time{ 16.7f };
		u32 render_item_count{ 0 };
		CameraID camera_id{ ID::Invalid_ID };
	};

	class Surface {
	public:
		constexpr explicit Surface(SurfaceID id) : _id{ id } {}
		constexpr Surface() = default;
		constexpr SurfaceID GetID() const { return _id; }
		const bool IsValid() const { return ID::IsValid(_id); }

		void Resize(u32, u32) const;
		const u32 Width() const;
		const u32 Height() const;
		void Render(FrameInfo) const;

	private:
		SurfaceID _id{ ID::Invalid_ID };
	};

	struct RenderSurface {
		Platform::Window window{};
		Zetta::Graphics::Surface surface{};
	};

	struct DirectionalLightParams {

	};

	struct PointLightParams {
		Math::v3 attenuation;
		f32 range;
	};

	struct SpotLightParams {
		Math::v3 attenuation;
		f32 range;
		// Umbra angle in radians [0, pi)
		f32 umbra;
		// Penumbra angle in radians [umbra, pi)
		f32 penumbra;
	};

	struct LightInitInfo {
		u64 light_set_key{ 0 };
		ID::ID_Type entity_id{ ID::Invalid_ID };
		Light::Type type{};
		f32 intensity{ 1.f };
		Math::v3 color{ 1.f, 1.f, 1.f };
		union {
			DirectionalLightParams directional_params;
			PointLightParams point_params;
			SpotLightParams spot_params;
		};
		bool is_enabled{ true };
	};

	struct LightParameter {
		enum Parameter : u32 {
			IsEnabled,
			Intensity,
			Color,
			Attenuation,
			Range,
			Umbra,
			Penumbra,
			Type,
			EntityID,

			count
		};
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
			near_z = 0.01f;
			far_z = 1000.f;
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
			near_z = 0.01f;
			far_z = 1000.f;
		}
	};

	struct ShaderFlags {
		enum Flags : u32 {
			None			= 0x00,
			Vertex			= 0x01,
			Hull			= 0x02,
			Domain			= 0x04,
			Geometry		= 0x08,
			Pixel			= 0x10,
			Compute			= 0x20,
			Amplification	= 0x40,
			Mesh			= 0x80,
		};
	};

	struct ShaderType {
		enum Type : u32 {
			Vertex = 0,
			Hull,
			Domain,
			Geometry,
			Pixel,
			Compute,
			Amplification,
			Mesh,

			count
		};
	};

	struct MaterialType {
		enum Type : u32 {
			Opaque,

			count
		};
	};

	struct MaterialInitInfo {
		MaterialType::Type type;
		u32 texture_count;
		ID::ID_Type shader_ids[ShaderType::count]{ ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID, ID::Invalid_ID };
		ID::ID_Type* texture_ids;
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

	Light CreateLight(LightInitInfo info);
	void RemoveLight(LightID id, u64 light_set_key);

	Camera CreateCamera(CameraInitInfo info);
	void RemoveCamera(CameraID id);

	ID::ID_Type AddSubmesh(const u8*& data);
	void RemoveSubmesh(ID::ID_Type id);

	ID::ID_Type AddMaterial(MaterialInitInfo info);
	void RemoveMaterial(ID::ID_Type id);

	ID::ID_Type AddRenderItem(ID::ID_Type entity_id, ID::ID_Type geometry_content_id,
		u32 mat_count, const ID::ID_Type* const mat_ids);
	void RemoveRenderItem(ID::ID_Type id);
}