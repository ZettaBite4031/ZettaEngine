#pragma once
#include "CommonHeaders.h"

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

struct ShaderFileInfo {
	const char* file;
	const char* function;
	ShaderType::type type;
};

std::unique_ptr<u8[]> CompileShadersSM66(ShaderFileInfo info, const char* file_path, Zetta::util::vector<std::wstring>& extra_args);

bool CompileShadersSM66();
