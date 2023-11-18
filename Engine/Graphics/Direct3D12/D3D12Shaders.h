#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Shaders {
	struct ShaderType {
		enum type : u32 {
			vertex = 0,
			hull,
			domain,
			geometry,
			pixel,
			compute,
			amplification,
			mesh,

			count
		};
	};

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