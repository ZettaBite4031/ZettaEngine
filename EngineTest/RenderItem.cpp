#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "ShaderCompilation.h"
#include "Components/Entity.h"

using namespace Zetta;

bool ReadFile(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size);

namespace {
	ID::ID_Type model_id{ ID::Invalid_ID };
	ID::ID_Type vs_id{ ID::Invalid_ID };
	ID::ID_Type ps_id{ ID::Invalid_ID };

	std::unordered_map<ID::ID_Type, ID::ID_Type> render_item_entity_map;

	void LoadModel() {
		std::unique_ptr<u8[]> model;
		u64 size{ 0 };
		ReadFile("..\\..\\EngineTest\\model.model", model, size);

		model_id = Content::CreateResource(model.get(), Content::AssetType::Mesh);
		assert(ID::IsValid(model_id));
	}

	void LoadShaders() {
		ShaderFileInfo info{};
		info.file = "TestShader.hlsl";
		info.function = "TestShaderVS";
		info.type = ShaderType::vertex;

		const char* shader_path{ "..\\..\\EngineTest\\" };

		auto vertex_shader = CompileShaders(info, shader_path);
		assert(vertex_shader.get());

		info.function = "TestShaderPS";
		info.type = ShaderType::pixel;

		auto pixel_shader = CompileShaders(info, shader_path);
		assert(pixel_shader.get());

		vs_id = Content::AddShader(vertex_shader.get());
		ps_id = Content::AddShader(pixel_shader.get());
	}

}

ID::ID_Type CreateRenderItem(ID::ID_Type entity_id) {
	// Load a model and pretend it belongs to an entity
	auto _1 = std::thread{ [] {LoadModel(); } };

	// load a material
	auto _2 = std::thread{ [] {LoadShaders(); } };

	_1.join();
	_2.join();

	// add render item using the model and its material(s).

	ID::ID_Type item_id{ 0 };

	render_item_entity_map[item_id] = entity_id;

	return { 0 };
}

void DestroyRenderItem(ID::ID_Type item_id) {
	if (ID::IsValid(item_id)) {
		auto pair = render_item_entity_map.find(item_id);
		if (pair != render_item_entity_map.end()) 
			GameEntity::RemoveGameEntity(GameEntity::EntityID{ pair->second });
	}

	if (ID::IsValid(vs_id)) Content::RemoveShader(vs_id);
	if (ID::IsValid(ps_id)) Content::RemoveShader(ps_id);
	if (ID::IsValid(model_id)) Content::DestroyResource(model_id, Content::AssetType::Mesh);

}