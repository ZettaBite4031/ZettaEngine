#include "D3D12Shaders.h"
#include "Content/ContentLoader.h"
#include "Content/ContentToEngine.h"

namespace Zetta::Graphics::D3D12::Shaders {
	namespace {
		

		Content::pCompiledShader engine_shaders[EngineShader::count];

		std::unique_ptr<u8[]> shader_blob{};

		bool LoadEngineShaders() {
			assert(!shader_blob);
			u64 size{ 0 };
			bool res = Content::LoadEngineShaders(shader_blob, size);
			assert(shader_blob && size);

			u64 offset{ 0 };
			u32 index{ 0 };
			while (offset < size && res) {
				assert(index < EngineShader::count);
				Content::pCompiledShader& shader{ engine_shaders[index] };
				assert(!shader);
				res &= index < EngineShader::count && !shader;
				if (!res) break;
				shader = reinterpret_cast<const Content::pCompiledShader>(&shader_blob[offset]);
				offset += shader->Size();
				index++;
			}
			assert(offset == size && index == EngineShader::count);

			return res;
		}
	}

	bool Initialize() {
		return LoadEngineShaders();
	}

	void Shutdown() {
		for (u32 i{ 0 }; i < EngineShader::count; i++) engine_shaders[i] = {};
		shader_blob.reset();
	}

	D3D12_SHADER_BYTECODE GetEngineShader(EngineShader::id id) {
		assert(id < EngineShader::count);
		const Content::pCompiledShader& shader{ engine_shaders[id] };
		assert(shader && shader->ByteCodeSize());
		return { shader->ByteCode(), shader->ByteCodeSize() };
	}
}