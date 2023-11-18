#include "ShaderCompilation.h"
#include "../packages/DirectXShaderCompiler/inc/d3d12shader.h"
#include "../packages/DirectXShaderCompiler/inc/dxcapi.h"
#include "Graphics/Direct3D12/D3D12Core.h"
#include "Graphics/Direct3D12/D3D12Shaders.h"

#include <fstream>
#include <filesystem>

#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace Zetta;
using namespace Zetta::Graphics::D3D12::Shaders;
using namespace Microsoft::WRL;

namespace {

	struct ShaderFileInfo {
		const char* file;
		const char* function;
		EngineShader::id id;
		ShaderType::type type;
	};

	constexpr ShaderFileInfo shader_files[]{
		{"FullScreenTriangle.hlsl", "FullScreenTriangleVS", EngineShader::FullscreenTriangleVS, ShaderType::vertex},
		{"FillColor.hlsl", "FillColorPS", EngineShader::FillColorPS, ShaderType::pixel},
		{"PostProcess.hlsl", "PostProcessPS", EngineShader::PostProcessPS, ShaderType::pixel},
	};

	static_assert(_countof(shader_files) == EngineShader::count);

	constexpr const char* shaders_source_path{ "../../Engine/Graphics/Direct3D12/Shaders/" };

	std::wstring ToWString(const char* c) {
		std::string s{ c };
		return { s.begin(), s.end() };
	}

	class ShaderCompiler {
	public:
		ShaderCompiler() { 
			HRESULT hr{ S_OK };
			DXCall(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
			if (FAILED(hr)) return;
			DXCall(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
			if (FAILED(hr)) return;
			DXCall(hr = _utils->CreateDefaultIncludeHandler(&_include_handler));
			if (FAILED(hr)) return;
		}

		DISABLE_COPY_AND_MOVE(ShaderCompiler);

		IDxcBlob* Compile(ShaderFileInfo info, std::filesystem::path full_path) {
			assert(_compiler && _utils && _include_handler);
			HRESULT hr{ S_OK };

			ComPtr<IDxcBlobEncoding> source_blob{ nullptr };
			// G:\Programming\Cpp\ZettaEngine\Engine\Graphics\Direct3D12\Shaders
			DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
			if (FAILED(hr)) return nullptr;
			assert(source_blob && source_blob->GetBufferSize());

			std::wstring file{ ToWString(info.file) };
			std::wstring func{ ToWString(info.function) };
			std::wstring prof{ ToWString(_profile_strings[(u32)info.type]) };
			std::wstring incl{ ToWString(shaders_source_path) };

			LPCWSTR args[]{
				file.c_str(),						// Optional shader  source file name for error reporting
				L"-E", func.c_str(),				// Entry function
				L"-T", prof.c_str(),				// Target function
				L"-I", incl.c_str(),				// Shader location
				DXC_ARG_ALL_RESOURCES_BOUND,
#if _DEBUG
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS,
#else
				DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
				DXC_ARG_WARNINGS_ARE_ERRORS,
				L"-Qstrip_reflect",					// Strip reflections into a separate blob
				L"-Qstrip_debug",					// Strip debug information into a separate blob
			};

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(info.file);

			return Compile(source_blob.Get(), args, _countof(args));


			return nullptr;
		}

		IDxcBlob* Compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args) {
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = source_blob->GetBufferPointer();
			buffer.Size = source_blob->GetBufferSize();

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> res{ nullptr };
			DXCall(hr = _compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&res)));
			if (FAILED(hr)) return nullptr;

			ComPtr<IDxcBlobUtf8> err{ nullptr };
			DXCall(hr = res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&err), nullptr));
			if (FAILED(hr)) return nullptr;

			if (err && err->GetStringLength()) {
				OutputDebugStringA("\nShader compilation error: \n");
				OutputDebugStringA(err->GetStringPointer());
			}
			else OutputDebugStringA(" [ SUCCEEDED ]");
			OutputDebugStringA("\n");

			HRESULT status{ S_OK };
			DXCall(hr = res->GetStatus(&status));
			if (FAILED(hr) || FAILED(status)) return nullptr;

			ComPtr<IDxcBlob> shader{ nullptr };
			DXCall(hr = res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			if (FAILED(hr)) return nullptr;

			return shader.Detach();
		}

	private:
		constexpr static const char* _profile_strings[]{ "vs_6_5","hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5" };
		static_assert(_countof(_profile_strings) == ShaderType::count);

		ComPtr<IDxcCompiler3> _compiler{};
		ComPtr<IDxcUtils> _utils{};
		ComPtr<IDxcIncludeHandler> _include_handler{};
	};

	decltype(auto) GetEngineShadersPath() { 
		return std::filesystem::path{ Graphics::GetEngineShadersPath(Graphics::GraphicsPlatform::Direct3D12) };
	}

	bool CompiledShadersUpToDate() {
		auto engine_shaders_path = GetEngineShadersPath();
		if (!std::filesystem::exists(engine_shaders_path)) return false;
		auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

		std::filesystem::path path{};
		std::filesystem::path full_path{};

		// Check if either of engine shader source files is newer than the compiled shader file.
		// In that case, we need to recompile.
		for (u32 i{ 0 }; i < EngineShader::count; ++i)
		{
			auto& info = shader_files[i];

			path = shaders_source_path;
			path += info.file;
			full_path = path;
			if (!std::filesystem::exists(full_path)) return false;

			auto shader_file_time = std::filesystem::last_write_time(full_path);
			if (shader_file_time > shaders_compilation_time) return false;
			
		}

		return true;
	}

	bool SaveCompiledShaders(util::vector<ComPtr<IDxcBlob>>& shaders) {
		auto engine_shaders_path = GetEngineShadersPath();
		std::filesystem::create_directories(engine_shaders_path.parent_path());
		std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
		if (!file || !std::filesystem::exists(engine_shaders_path)) { file.close(); return false; }

		for (auto& shader : shaders) {
			const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
			file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
			file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
		}

		file.close();
		return true;
	}
}

bool CompileShaders() {
	if (CompiledShadersUpToDate()) return true;

	util::vector<ComPtr<IDxcBlob>> shaders;
	std::filesystem::path path{};
	std::filesystem::path full_path{};

	ShaderCompiler compiler{};
	for (u32 i{ 0 }; i < EngineShader::count; i++) {
		auto& info = shader_files[i];
		path = shaders_source_path;
		path += info.file;
		full_path = std::filesystem::absolute(path);
		if (!std::filesystem::exists(full_path)) return false;
		ComPtr<IDxcBlob> compiled_shader{ compiler.Compile(info, full_path) };
		if (!compiled_shader || !compiled_shader->GetBufferSize() || !compiled_shader->GetBufferPointer()) return false;
		shaders.emplace_back(std::move(compiled_shader));
	}

	return SaveCompiledShaders(shaders);
}