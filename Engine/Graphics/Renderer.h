#pragma once
#include "CommonHeaders.h"
#include "../Platform/Window.h"

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
}