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
		{ EngineShader::FullscreenTriangleVS, {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", ShaderType::vertex} },
		{ EngineShader::FillColorPS, {"FillColor.hlsl", "FillColorPS", ShaderType::pixel} },
		{ EngineShader::PostProcessPS, {"PostProcess.hlsl", "PostProcessPS", ShaderType::pixel} },
		{ EngineShader::GridFrustumCS, {"GridFrustums.hlsl", "ComputeGridFrustumsCS", ShaderType::compute} },
		{ EngineShader::CullLightsCS, {"CullLights.hlsl", "CullLightsCS", ShaderType::compute} },
	};

	static_assert(_countof(engine_shader_files) == EngineShader::count);

	constexpr const char* shaders_source_pathSM66{ "../../Engine/Graphics/Direct3D12/Shaders/" };

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

		DXCCompiledShader CompileSM66(ShaderFileInfo info, std::filesystem::path full_path, util::vector<std::wstring>& extra_args) {
			assert(_compiler && _utils && _include_handler);
			HRESULT hr{ S_OK };

			ComPtr<IDxcBlobEncoding> source_blob{ nullptr };
			// G:\Programming\Cpp\ZettaEngine\Engine\Graphics\Direct3D12\Shaders
			DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
			if (FAILED(hr)) return {};
			assert(source_blob && source_blob->GetBufferSize());

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(info.file);
			OutputDebugStringA(" : ");
			OutputDebugStringA(info.function);
			OutputDebugStringA("\n");

			return Compile(source_blob.Get(), GetArgsSM66(info, extra_args));
		}

		DXCCompiledShader Compile(IDxcBlobEncoding* source_blob, util::vector<std::wstring> compiler_args) {
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = source_blob->GetBufferPointer();
			buffer.Size = source_blob->GetBufferSize();

			util::vector<LPCWSTR> args;
			for (const auto& arg : compiler_args) args.emplace_back(arg.c_str());

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> res{ nullptr };
			DXCall(hr = _compiler->Compile(&buffer, args.data(), (u32)args.size(), _include_handler.Get(), IID_PPV_ARGS(&res)));
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
		util::vector<std::wstring> GetArgsSM66(const ShaderFileInfo info, util::vector<std::wstring>& extra_args) {
			util::vector<std::wstring> args{};

			args.emplace_back(ToWString(info.file));
			args.emplace_back(L"-E");
			args.emplace_back(ToWString(info.function));
			args.emplace_back(L"-T");
			args.emplace_back(ToWString(_profile_strings[(u32)info.type]));
			args.emplace_back(L"-I");
			args.emplace_back(ToWString(shaders_source_pathSM66));
			args.emplace_back(L"-enable-16bit-types");
			args.emplace_back(DXC_ARG_ALL_RESOURCES_BOUND);
#ifdef _DEBUG
			args.emplace_back(DXC_ARG_DEBUG);
			args.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
			args.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif
			args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS);
			args.emplace_back(L"-Qstrip_reflect");
			args.emplace_back(L"-Qstrip_debug");
			
			for (const auto& arg : extra_args)
				args.emplace_back(arg.c_str());

			return args;
		}

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

		for (const auto& entry : std::filesystem::directory_iterator{ shaders_source_pathSM66 }) 
			if (entry.last_write_time() > shaders_compilation_time) return false;
		
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

std::unique_ptr<u8[]> CompileShadersSM66(ShaderFileInfo info, const char* file_path, Zetta::util::vector<std::wstring>& extra_args) {
	std::filesystem::path full_path{ file_path };
	full_path += info.file;
	if (!std::filesystem::exists(full_path)) return {};

	// NOTE: According to marcelolr (https://github.com/Microsoft/DirectXShaderCompiler/issues/79)
	//		 "... creating compiler instances is pretty cheap, so it's probably not worth the hassle of caching / sarding them."
	ShaderCompiler compiler{};
	DXCCompiledShader compiled_shader{ compiler.CompileSM66(info, full_path, extra_args) };

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

bool CompileShadersSM66() {
	if (CompiledShadersUpToDate()) return true;

	ShaderCompiler compiler{};

	util::vector<DXCCompiledShader> shaders;
	std::filesystem::path full_path{};
	for (u32 i{ 0 }; i < EngineShader::count; i++) {
		auto& file = engine_shader_files[i];

		full_path = shaders_source_pathSM66;
		full_path += file.info.file;
		if (!std::filesystem::exists(full_path)) return false;
		util::vector<std::wstring> extra_args{};

		if (file.id == EngineShader::GridFrustumCS || file.id == EngineShader::CullLightsCS) {
			// TODO: Get TILE_SIZE value from D3D12
			extra_args.emplace_back(L"-D");
			extra_args.emplace_back(L"TILE_SIZE=32");
		}

		DXCCompiledShader compiled_shader{ compiler.CompileSM66(file.info, full_path, extra_args) };
		if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferSize() && compiled_shader.byte_code->GetBufferPointer())
			shaders.emplace_back(std::move(compiled_shader));
		else return false;
	}

	return SaveCompiledShaders(shaders);
}