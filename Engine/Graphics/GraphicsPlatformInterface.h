#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"

namespace Zetta::Graphics {
	struct PlatformInterface {
		bool(*Initialize)(void);
		void(*Shutdown)(void);

		struct {
			Surface(*Create)(Platform::Window);
			void(*Remove)(SurfaceID);
			void(*Resize)(SurfaceID, u32, u32);
			u32(*Width)(SurfaceID);
			u32(*Height)(SurfaceID);
			void(*Render)(SurfaceID);
		} Surface;

		GraphicsPlatform platform = (GraphicsPlatform)-1;
	};
}