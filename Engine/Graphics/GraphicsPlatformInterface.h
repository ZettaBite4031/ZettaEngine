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
			void(*Render)(SurfaceID, FrameInfo);
		} Surface;

		struct {
			Camera(*Create)(CameraInitInfo);
			void(*Remove)(CameraID);
			void(*SetParameter)(CameraID, CameraParameter::Parameter, const void* const, u32);
			void(*GetParameter)(CameraID, CameraParameter::Parameter, void* const, u32);
		} Camera;

		struct {
			Light(*Create)(LightInitInfo);
			void(*Remove)(LightID, u64);
			void(*SetParameter)(LightID, u64, LightParameter::Parameter, const void* const, u32);
			void(*GetParameter)(LightID, u64, LightParameter::Parameter, void* const, u32);
		} Light;

		struct {
			ID::ID_Type(*AddSubmesh)(const u8*&);
			void(*RemoveSubmesh)(ID::ID_Type);
			ID::ID_Type(*AddMaterial)(const MaterialInitInfo);
			void(*RemoveMaterial)(ID::ID_Type);
			ID::ID_Type(*AddRenderItem)(ID::ID_Type, ID::ID_Type, u32, const ID::ID_Type* const);
			void(*RemoveRenderItem)(ID::ID_Type);
		} Resources;

		GraphicsPlatform platform = (GraphicsPlatform)-1;
	};
}