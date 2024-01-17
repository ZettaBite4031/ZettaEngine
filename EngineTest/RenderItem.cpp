#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "ShaderCompilation.h"
#include "Components/Entity.h"
#include "../ContentToolsDLL/Geometry.h"

using namespace Zetta;

bool ReadFile(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size);
GameEntity::Entity CreateGameEntity(Math::v3 position, Math::v3 rotation, const char* script_name);
#ifndef _DEBUG
void RemoveGameEntity(GameEntity::EntityID id);
#endif

namespace {
	ID::ID_Type fan_model_id{ ID::Invalid_ID };
	ID::ID_Type ship_model_id{ ID::Invalid_ID };
	ID::ID_Type lab_model_id{ ID::Invalid_ID };
	
	ID::ID_Type fan_item_id{ ID::Invalid_ID };
	ID::ID_Type ship_item_id{ ID::Invalid_ID };
	ID::ID_Type lab_item_id{ ID::Invalid_ID };

	GameEntity::EntityID fan_entity_id{ ID::Invalid_ID };
	GameEntity::EntityID ship_entity_id{ ID::Invalid_ID };
	GameEntity::EntityID lab_entity_id{ ID::Invalid_ID };

	ID::ID_Type vs_id{ ID::Invalid_ID };
	ID::ID_Type ps_id{ ID::Invalid_ID };
	ID::ID_Type mat_id{ ID::Invalid_ID };

	std::unordered_map<ID::ID_Type, GameEntity::EntityID> render_item_entity_map;

	[[nodiscard]] ID::ID_Type LoadModel(const char* path) {
		std::unique_ptr<u8[]> model;
		u64 size{ 0 };
		ReadFile(path, model, size);

		const ID::ID_Type model_id{ Content::CreateResource(model.get(), Content::AssetType::Mesh) };
		assert(ID::IsValid(model_id));
		return model_id;
	}

	void LoadShaders() {
		ShaderFileInfo info{};
		info.file = "TestShader.hlsl";
		info.function = "TestShaderVS";
		info.type = ShaderType::vertex;

		const char* shader_path{ "..\\..\\EngineTest\\" };

		std::wstring defines[]{ L"ELEMENTS_TYPE=1", L"ELEMENTS_TYPE=3"};
		util::vector<u32> keys;
		keys.emplace_back((u32)Tools::Elements::ElementsType::StaticNormal);
		keys.emplace_back((u32)Tools::Elements::ElementsType::StaticNormalTexture);
		util::vector<std::wstring> extra_args{};
		util::vector<std::unique_ptr<u8[]>> vertex_shaders;
		util::vector<const u8*> vertex_shader_pointers;
		for (u32 i{ 0 }; i < _countof(defines); i++) {
			extra_args.clear();
			extra_args.emplace_back(L"-D");
			extra_args.emplace_back(defines[i]);
			vertex_shaders.emplace_back(std::move(CompileShadersSM66(info, shader_path, extra_args)));
			assert(vertex_shaders.back().get());
			vertex_shader_pointers.emplace_back(vertex_shaders.back().get());
		}

		extra_args.clear();
		info.function = "TestShaderPS";
		info.type = ShaderType::pixel;

		auto pixel_shader = CompileShadersSM66(info, shader_path, extra_args);
		assert(pixel_shader.get());

		vs_id = Content::AddShaderGroup(vertex_shader_pointers.data(), (u32)vertex_shader_pointers.size(), keys.data());
		const u8* pixel_shaders[]{ pixel_shader.get() };
		ps_id = Content::AddShaderGroup(&pixel_shaders[0], 1, &u32_invalid_id);
	}

	void CreateMaterial() {
		assert(ID::IsValid(vs_id) && ID::IsValid(ps_id));
		Graphics::MaterialInitInfo info{};
		info.shader_ids[Graphics::ShaderType::Vertex] = vs_id;
		info.shader_ids[Graphics::ShaderType::Pixel] = ps_id;
		info.type = Graphics::MaterialType::Opaque;
		mat_id = Content::CreateResource(&info, Content::AssetType::Material);
	}

	void RemoveItem(ID::ID_Type item_id, ID::ID_Type model_id) {
		if (ID::IsValid(item_id)) {
			Graphics::RemoveRenderItem(item_id);
			auto pair = render_item_entity_map.find(item_id);
			if (pair != render_item_entity_map.end()) {
				RemoveGameEntity(pair->second);
			}

			if (ID::IsValid(model_id)) Content::DestroyResource(model_id, Content::AssetType::Mesh);
			
		}
	}
}

void CreateRenderItems() {
	// Load a model and pretend it belongs to an entity
	auto _1 = std::thread{ [] { lab_model_id = LoadModel("..\\..\\x64\\lab_model.model"); } };
	auto _2 = std::thread{ [] { fan_model_id = LoadModel("..\\..\\x64\\fan_model.model"); } };
	auto _3 = std::thread{ [] { ship_model_id = LoadModel("..\\..\\x64\\ship_model.model"); } };
	auto _4 = std::thread{ [] {LoadShaders(); } };

	lab_entity_id = CreateGameEntity({}, {}, nullptr).GetID();
	fan_entity_id = CreateGameEntity({ -10.47f, 5.93f, -6.7f }, {}, "FanScript").GetID();
	ship_entity_id = CreateGameEntity({ 0.f, 1.3f, -6.6f }, {}, "ShipScript").GetID();

	_1.join();
	_2.join();
	_3.join();
	_4.join();

	// add render item using the model and its material(s).
	CreateMaterial();
	ID::ID_Type materials[]{ mat_id };

	lab_item_id = Graphics::AddRenderItem(lab_entity_id, lab_model_id, _countof(materials), &materials[0]);
	fan_item_id = Graphics::AddRenderItem(fan_entity_id, fan_model_id, _countof(materials), &materials[0]);
	ship_item_id = Graphics::AddRenderItem(ship_entity_id, ship_model_id, _countof(materials), &materials[0]);

	render_item_entity_map[lab_item_id] = lab_entity_id;
	render_item_entity_map[fan_item_id] = fan_entity_id;
	render_item_entity_map[ship_item_id] = ship_entity_id;

}

void DestroyRenderItems() {
	RemoveItem(lab_item_id, lab_model_id);
	RemoveItem(fan_item_id, fan_model_id);
	RemoveItem(ship_item_id, ship_model_id);

	if (ID::IsValid(mat_id)) Content::DestroyResource(mat_id, Content::AssetType::Material);
	
	if (ID::IsValid(vs_id)) Content::RemoveShaderGroup(vs_id);
	if (ID::IsValid(ps_id)) Content::RemoveShaderGroup(ps_id);

}

void GetRenderItems(ID::ID_Type* items, [[maybe_unused]] u32 count) {
	assert(count == 3);
	items[0] = lab_item_id;
	items[1] = fan_item_id;
	items[2] = ship_item_id;
}