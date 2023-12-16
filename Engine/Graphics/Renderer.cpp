#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"
#include "EngineAPI/Camera.h"

namespace Zetta::Graphics {
	namespace {
		constexpr const char* engine_shader_path[]{
			".\\Shaders\\D3D12\\Shaders.bin",
//			".\\Shaders\\Vulkan\\Shaders.bin"
		};

		PlatformInterface gfx{};

		bool SetPlatformInterface(GraphicsPlatform platform, PlatformInterface& pi) {
			switch (platform) {
			case GraphicsPlatform::Direct3D12:
				D3D12::GetPlatformInterface(pi);
				break;
			default:
				return false;
			}
			assert(pi.platform == platform);
			return true;
		}
	}

	bool Initialize(GraphicsPlatform platform) {
		return SetPlatformInterface(platform, gfx) && gfx.Initialize();
	}

	void Shutdown() {
		if (gfx.platform != (GraphicsPlatform) - 1 ) gfx.Shutdown();
	}

	const char* GetEngineShadersPath() {
		return engine_shader_path[(u32)gfx.platform];
	}


	const char* GetEngineShadersPath(GraphicsPlatform platform) {
		return engine_shader_path[(u32)platform];
	}

	Surface CreateSurface(Platform::Window window) {
		return gfx.Surface.Create(window);
	}

	void RemoveSurface(SurfaceID id) {
		assert(ID::IsValid(id));
		gfx.Surface.Remove(id);
	}

	void Surface::Resize(u32 width, u32 height) const {
		assert(IsValid());
		gfx.Surface.Resize(_id, width, height);
	}

	const u32 Surface::Width() const {
		assert(IsValid());
		return gfx.Surface.Width(_id);
	}

	const u32 Surface::Height() const {
		assert(IsValid());
		return gfx.Surface.Height(_id);
	}

	void Surface::Render(FrameInfo info) const {
		assert(IsValid());
		gfx.Surface.Render(_id, info);
	}

	void Camera::up(Math::v3 up) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::UpVector, &up, sizeof(up));
	}

	void Camera::FOV(f32 fov) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::FOV, &fov, sizeof(fov));
	}

	void Camera::AspectRatio(f32 aspect_ratio) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::AspectRatio, &aspect_ratio, sizeof(aspect_ratio));
	}

	void Camera::ViewWidth(f32 width) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::ViewWidth, &width, sizeof(width));
	}

	void Camera::ViewHeight(f32 height) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::ViewHeight, &height, sizeof(height));
	}

	void Camera::Range(f32 nearZ, f32 farZ) const {
		assert(IsValid());
		gfx.Camera.SetParameter(_id, CameraParameter::NearZ, &nearZ, sizeof(nearZ));
		gfx.Camera.SetParameter(_id, CameraParameter::FarZ, &farZ, sizeof(farZ));
	}

	Math::mat4 Camera::View() const {
		assert(IsValid());
		Math::mat4 mat;
		gfx.Camera.GetParameter(_id, CameraParameter::View, &mat, sizeof(mat));
		return mat;
	}

	Math::mat4 Camera::Projection() const {
		assert(IsValid());
		Math::mat4 mat;
		gfx.Camera.GetParameter(_id, CameraParameter::Projection, &mat, sizeof(mat));
		return mat;
	}

	Math::mat4 Camera::InverseProjection() const {
		assert(IsValid());
		Math::mat4 mat;
		gfx.Camera.GetParameter(_id, CameraParameter::InverseProjection, &mat, sizeof(mat));
		return mat;
	}

	Math::mat4 Camera::ViewProjection() const {
		assert(IsValid());
		Math::mat4 mat;
		gfx.Camera.GetParameter(_id, CameraParameter::ViewProjection, &mat, sizeof(mat));
		return mat;
	}

	Math::mat4 Camera::InverseViewProjection() const {
		assert(IsValid());
		Math::mat4 mat;
		gfx.Camera.GetParameter(_id, CameraParameter::InverseProjection, &mat, sizeof(mat));
		return mat;
	}

	Math::v3 Camera::up() const {
		assert(IsValid());
		Math::v3 v;
		gfx.Camera.GetParameter(_id, CameraParameter::UpVector, &v, sizeof(v));
		return v;
	}

	f32 Camera::NearZ() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::NearZ, &res, sizeof(res));
		return res;
	}

	f32 Camera::FarZ() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::FarZ, &res, sizeof(res));
		return res;
	}

	f32 Camera::FOV() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::FOV, &res, sizeof(res));
		return res;
	}

	f32 Camera::AspectRatio() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::AspectRatio, &res, sizeof(res));
		return res;
	}

	f32 Camera::ViewWidth() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::ViewWidth, &res, sizeof(res));
		return res;
	}

	f32 Camera::ViewHeight() const {
		assert(IsValid());
		f32 res;
		gfx.Camera.GetParameter(_id, CameraParameter::ViewHeight, &res, sizeof(res));
		return res;
	}

	Camera::Type Camera::ProjectionType() const {
		assert(IsValid());
		Type type;
		gfx.Camera.GetParameter(_id, CameraParameter::Type, &type, sizeof(type));
		return type;
	}

	ID::ID_Type Camera::EntityID() const {
		assert(IsValid());
		ID::ID_Type entity_id;
		gfx.Camera.GetParameter(_id, CameraParameter::EntityID, &entity_id, sizeof(entity_id));
		return entity_id;
	}

	Camera CreateCamera(CameraInitInfo info) {
		return gfx.Camera.Create(info);
	}

	void RemoveCamera(CameraID id) {
		gfx.Camera.Remove(id);
	}

	void Light::IsEnabled(bool is_enabled) const {
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::IsEnabled, &is_enabled, sizeof(is_enabled));
	}

	void Light::Intensity(f32 intensity) const {
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Intensity, &intensity, sizeof(intensity)); 
	}

	void Light::Color(Math::v3 color) const {
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Color, &color, sizeof(color));
	}

	void Light::Attenuation(Math::v3 attenuation) const{
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Attenuation, &attenuation, sizeof(attenuation));
	}

	void Light::Range(f32 range) const{
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Range, &range, sizeof(range));
	}

	void Light::ConeAngles(f32 umbra, f32 penumbra) const {
		assert(IsValid());
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Umbra, &umbra, sizeof(umbra));
		gfx.Light.SetParameter(_id, _light_set_key, LightParameter::Penumbra, &penumbra, sizeof(penumbra));
	}

	bool Light::IsEnabled() const {
		assert(IsValid());
		bool is_enabled;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::IsEnabled, &is_enabled, sizeof(is_enabled));
		return is_enabled;
	}

	f32 Light::Intensity() const {
		assert(IsValid());
		f32 intensity;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Intensity, &intensity, sizeof(intensity));
		return intensity;
	}

	Math::v3 Light::Color() const {
		assert(IsValid());
		Math::v3 color;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Color, &color, sizeof(color));
		return color;
	}

	Math::v3 Light::Attenuation() const {
		assert(IsValid());
		Math::v3 attenuation;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Attenuation, &attenuation, sizeof(attenuation));
		return attenuation;
	}

	f32 Light::Range() const {
		assert(IsValid());
		f32 range;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Range, &range, sizeof(range));
		return range;
	}

	f32 Light::Umbra() const {
		assert(IsValid());
		f32 umbra;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Umbra, &umbra, sizeof(umbra));
		return umbra;
	}

	f32 Light::Penumbra() const {
		assert(IsValid());
		f32 penumbra;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Penumbra, &penumbra, sizeof(penumbra));
		return penumbra;
	}

	Light::Type Light::LightType() const {
		assert(IsValid());
		Light::Type type;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::Type, &type, sizeof(type));
		return type;
	}

	ID::ID_Type Light::EntityID() const {
		assert(IsValid());
		ID::ID_Type id;
		gfx.Light.GetParameter(_id, _light_set_key, LightParameter::EntityID, &id, sizeof(id));
		return id;
	}

	Light CreateLight(LightInitInfo info) {
		return gfx.Light.Create(info);
	}

	void RemoveLight(LightID id, u64 light_set_key) {
		gfx.Light.Remove(id, light_set_key);
	}

	ID::ID_Type AddSubmesh(const u8*& data) {
		return gfx.Resources.AddSubmesh(data);
	}

	void RemoveSubmesh(ID::ID_Type id) {
		gfx.Resources.RemoveSubmesh(id);
	}

	ID::ID_Type AddMaterial(MaterialInitInfo info) {
		return gfx.Resources.AddMaterial(info);
	}

	void RemoveMaterial(ID::ID_Type id) {
		gfx.Resources.RemoveMaterial(id);
	}

	ID::ID_Type AddRenderItem(ID::ID_Type entity_id, ID::ID_Type geometry_content_id,
		u32 mat_count, const ID::ID_Type* const mat_ids) {
		return gfx.Resources.AddRenderItem(entity_id, geometry_content_id, mat_count, mat_ids);
	}

	void RemoveRenderItem(ID::ID_Type id) {
		gfx.Resources.RemoveRenderItem(id);
	}
}