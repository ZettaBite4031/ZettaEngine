#include "ContentLoader.h"

#include "../Components/Entity.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

#if !defined(SHIPPING)
#include <fstream>
#include <filesystem>
#include <Windows.h>

namespace Zetta::Content {
	namespace {
		enum ComponentType {
			Transform,
			Script,

			count
		};

		util::vector<GameEntity::Entity> entities;
		Transform::InitInfo transform_info{};
		Script::InitInfo script_info{};

		bool ReadTransform(const u8*& data, GameEntity::EntityInfo& info) {
			using namespace DirectX;
			f32 rotation[3];

			assert(!info.transform);
			memcpy(&transform_info.position[0], data, sizeof(transform_info.position)); data += sizeof(transform_info.position);
			memcpy(&rotation[0], data, sizeof(rotation)); data += sizeof(rotation);
			memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale)); data += sizeof(transform_info.scale);

			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);
			memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));

			info.transform = &transform_info;

			return true;
		}

		bool ReadScript(const u8*& data, GameEntity::EntityInfo& info) {
			assert(!info.script);
			const u32 name_length{ *data }; data += sizeof(u32);
			if (!name_length) return false;

			assert(name_length < 256);
			char script_name[256];
			memcpy(&script_name[0], data, name_length); data += name_length;
			script_name[name_length] = 0; // Zero-Terminated C-String
			script_info.script_creator = Script::Detail::GetScriptCreatorInternal(Script::Detail::StringHash()(script_name));
			info.script = &script_info;
			return script_info.script_creator != nullptr;
		}

		using ComponentReader = bool(*)(const u8*&, GameEntity::EntityInfo&);
		ComponentReader component_readers[]{ ReadTransform, ReadScript };
		static_assert(_countof(component_readers) == ComponentType::count);
	}

	bool LoadGame() {
		// set the working directory to exe path
		wchar_t path[MAX_PATH];
		const u32 length{ GetModuleFileName(0, &path[0], MAX_PATH) };
		if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return false;
		std::filesystem::path p{ path };
		SetCurrentDirectory(p.parent_path().wstring().c_str());

		// read game.bin and create the entities
		std::ifstream game("game.bin", std::ios::in | std::ios::binary);
		util::vector<u8> buffer(std::istreambuf_iterator<char>(game), {});
		assert(buffer.size());
		const u8* at{ buffer.data() };
		constexpr u32 su32{ sizeof(u32) };
		const u32 num_entities{ *at }; at += su32;
		if (!num_entities) return false;

		for (u32 entity_index{ 0 }; entity_index < num_entities; entity_index++) {
			GameEntity::EntityInfo info{};
			const u32 entity_type{ *at }; at += su32;
			const u32 num_components{ *at }; at += su32;
			if (!num_components) return false;

			for (u32 component_index{ 0 }; component_index < num_components; component_index++) {
				const u32 component_type{ *at }; at += su32;
				assert(component_type < ComponentType::count);
				if (!component_readers[component_type](at, info)) return false;
			}

			assert(info.transform);
			GameEntity::Entity entity{ GameEntity::CreateGameEntity(info) };
			if (!entity.IsValid()) return false;
			entities.emplace_back(entity);
		}

		assert(at == buffer.data() + buffer.size());
		return true;
	}

	void UnloadGame() {
		for (auto entity : entities) GameEntity::RemoveGameEntity(entity.GetID());
	}
}
#endif