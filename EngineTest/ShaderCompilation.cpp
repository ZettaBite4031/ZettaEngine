#include <fstream>
#include <filesystem>

#include "ShaderCompilation.h"
#include "../packages/DirectXShaderCompiler/inc/d3d12shader.h"
#include "../packages/DirectXShaderCompiler/inc/dxcapi.h"

#include "Graphics/Direct3D12/D3D12Core.h"
#include "Graphics/Direct3D12/D3D12Shaders.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"

#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace Zetta;
using namespace Zetta::Graphics::D3D12::Shaders;
using namespace Microsoft::WRL;

namespace {

	struct EngineShaderInfo {
		EngineShader::id id;
		ShaderFileInfo info;
	};

	constexpr EngineShaderInfo engine_shader_files[]{
		EngineShader::FullscreenTriangleVS, {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", ShaderType::vertex},
		EngineShader::FillColorPS, {"FillColor.hlsl", "FillColorPS", ShaderType::pixel},
		EngineShader::PostProcessPS, {"PostProcess.hlsl", "PostProcessPS", ShaderType::pixel},
	};

	static_assert(_countof(engine_shader_files) == EngineShader::count);

	constexpr const char* shaders_source_path{ "../../Engine/Graphics/Direct3D12/Shaders/" };

	struct DXCCompiledShader {
		ComPtr<IDxcBlob> byte_code;
		ComPtr<IDxcBlobUtf8> disassembly;
		DxcShaderHash hash;
	};

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

		DXCCompiledShader Compile(ShaderFileInfo info, std::filesystem::path full_path) {
			assert(_compiler && _utils && _include_handler);
			HRESULT hr{ S_OK };

			ComPtr<IDxcBlobEncoding> source_blob{ nullptr };
			// G:\Programming\Cpp\ZettaEngine\Engine\Graphics\Direct3D12\Shaders
			DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
			if (FAILED(hr)) return {};
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
				L"-enable-16bit-types",
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
			OutputDebugStringA(" : ");
			OutputDebugStringA(info.function);
			OutputDebugStringA("\n");

			return Compile(source_blob.Get(), args, _countof(args));
		}

		DXCCompiledShader Compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args) {
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = source_blob->GetBufferPointer();
			buffer.Size = source_blob->GetBufferSize();

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> res{ nullptr };
			DXCall(hr = _compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&res)));
			if (FAILED(hr)) return {};

			ComPtr<IDxcBlobUtf8> err{ nullptr };
			DXCall(hr = res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&err), nullptr));
			if (FAILED(hr)) return {};

			if (err && err->GetStringLength()) {
				OutputDebugStringA("\nShader compilation error: \n");
				OutputDebugStringA(err->GetStringPointer());
			}
			else OutputDebugStringA(" [ SUCCEEDED ]");
			OutputDebugStringA("\n");

			HRESULT status{ S_OK };
			DXCall(hr = res->GetStatus(&status));
			if (FAILED(hr) || FAILED(status)) return {};

			ComPtr<IDxcBlob> hash{ };
			DXCall(hr = res->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&hash), nullptr));
			if (FAILED(hr)) return {};
			DxcShaderHash* const hash_buffer{ (DxcShaderHash* const)hash->GetBufferPointer() };
			assert(!(hash_buffer->Flags & DXC_HASHFLAG_INCLUDES_SOURCE));
			OutputDebugStringA("Shader Hash: ");
			for (u32 i{ 0 }; i < _countof(hash_buffer->HashDigest); i++) {
				char hash_bytes[3]{};
				sprintf_s(hash_bytes, "%02x", (u32)hash_buffer->HashDigest[i]);
				OutputDebugStringA(hash_bytes);
				OutputDebugStringA(" ");
			}
			OutputDebugStringA("\n");

			ComPtr<IDxcBlob> shader{ nullptr };
			DXCall(hr = res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			if (FAILED(hr)) return {};
			buffer.Ptr = shader->GetBufferPointer();
			buffer.Size = shader->GetBufferSize();

			ComPtr<IDxcResult> disassembly_result{ nullptr };
			DXCall(hr = _compiler->Disassemble(&buffer, IID_PPV_ARGS(&disassembly_result)));

			ComPtr<IDxcBlobUtf8> disassembly{ nullptr };
			DXCall(hr = disassembly_result->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(&disassembly), nullptr));

			DXCCompiledShader shader_res{ shader.Detach(), disassembly.Detach() };
			memcpy(&shader_res.hash.HashDigest[0], &hash_buffer->HashDigest[0], _countof(hash_buffer->HashDigest));

			return shader_res;
		}

	private:
		constexpr static const char* _profile_strings[]{ "vs_6_6","hs_6_6", "ds_6_6", "gs_6_6", "ps_6_6", "cs_6_6", "as_6_6", "ms_6_6" };
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

		std::filesystem::path full_path{};

		// Check if either of engine shader source files is newer than the compiled shader file.
		// In that case, we need to recompile.
		for (u32 i{ 0 }; i < EngineShader::count; ++i)
		{
			auto& file = engine_shader_files[i];

			full_path = shaders_source_path;
			full_path += file.info.file;
			if (!std::filesystem::exists(full_path)) return false;

			auto shader_file_time = std::filesystem::last_write_time(full_path);
			if (shader_file_time > shaders_compilation_time) return false;
			
		}

		return true;
	}

	bool SaveCompiledShaders(util::vector<DXCCompiledShader>& shaders) {
		auto engine_shaders_path = GetEngineShadersPath();
		std::filesystem::create_directories(engine_shaders_path.parent_path());
		std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
		if (!file || !std::filesystem::exists(engine_shaders_path)) { file.close(); return false; }

		for (auto& shader : shaders) {
			const D3D12_SHADER_BYTECODE byte_code{ shader.byte_code->GetBufferPointer(), shader.byte_code->GetBufferSize() };
			file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
			file.write((char*)&shader.hash.HashDigest[0], _countof(shader.hash.HashDigest));
			file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
		}

		file.close();
		return true;
	}
}

std::unique_ptr<u8[]> CompileShaders(ShaderFileInfo info, const char* file_path) {
	std::filesystem::path full_path{ file_path };
	full_path += info.file;
	if (!std::filesystem::exists(full_path)) return {};

	// NOTE: According to marcelolr (https://github.com/Microsoft/DirectXShaderCompiler/issues/79)
	//		 "... creating compiler instances is pretty cheap, so it's probably not worth the hassle of caching / sarding them."
	ShaderCompiler compiler{};
	DXCCompiledShader compiled_shader{ compiler.Compile(info, full_path) };

	if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferPointer() && compiled_shader.byte_code->GetBufferSize()) {
		static_assert(Content::CompiledShader::hash_length == _countof(DxcShaderHash::HashDigest));
		const u64 buffer_size{ sizeof(u64) + Content::CompiledShader::hash_length + compiled_shader.byte_code->GetBufferSize() };
		std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(buffer_size) };
		util::BlobStreamWriter blob{ buffer.get(), buffer_size};
		blob.write(compiled_shader.byte_code->GetBufferSize());
		blob.write(compiled_shader.hash.HashDigest, Content::CompiledShader::hash_length);
		blob.write((u8*)compiled_shader.byte_code->GetBufferPointer(), compiled_shader.byte_code->GetBufferSize());

		assert(blob.Offset() == buffer_size);
		return buffer;
	}
	return {};
}

bool CompileShaders() {
	if (CompiledShadersUpToDate()) return true;

	ShaderCompiler compiler{};

	util::vector<DXCCompiledShader> shaders;
	std::filesystem::path full_path{};
	for (u32 i{ 0 }; i < EngineShader::count; i++) {
		auto& file = engine_shader_files[i];

		full_path = shaders_source_path;
		full_path += file.info.file;
		if (!std::filesystem::exists(full_path)) return false;

		DXCCompiledShader compiled_shader{ compiler.Compile(file.info, full_path) };
		if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferSize() && compiled_shader.byte_code->GetBufferPointer())
			shaders.emplace_back(std::move(compiled_shader));
		else return false;
	}

	return SaveCompiledShaders(shaders);
}