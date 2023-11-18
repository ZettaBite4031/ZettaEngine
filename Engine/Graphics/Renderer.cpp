#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace Zetta::Graphics {
	namespace {
		constexpr const char* engine_shader_path[]{
			".\\Shaders\\D3D12\\Shaders.bin",
//			".\\Shaders\\Vulkan\\Shaders.bin"
		};

		PlatformInterface gfx{};

		bool SetPlatformInterface(GraphicsPlatform platform) {
			switch (platform) {
			case GraphicsPlatform::Direct3D12:
				D3D12::GetPlatformInterface(gfx);
				break;
			default:
				return false;
			}
			assert(gfx.platform == platform);
			return true;
		}
	}

	bool Initialize(GraphicsPlatform platform) {
		return SetPlatformInterface(platform) && gfx.Initialize();
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

	void Surface::Render() const {
		assert(IsValid());
		gfx.Surface.Render(_id);
	}


}