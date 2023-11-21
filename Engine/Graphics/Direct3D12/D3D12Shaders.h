#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Shaders {
	struct EngineShader {
		enum id : u32 {
			FullscreenTriangleVS = 0,
			FillColorPS = 1,
			PostProcessPS = 2,

			count
		};
	};

	bool Initialize();
	void Shutdown();

	D3D12_SHADER_BYTECODE GetEngineShader(EngineShader::id id);
}