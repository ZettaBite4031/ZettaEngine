#include "Script.h"
#include "Entity.h"
#include "Transform.h"

#define USE_TRANSFORM_CACHE_MAP 1

namespace Zetta::Script {
	namespace {
		util::vector<Detail::ScriptPtr>				entity_scripts;
		util::vector<ID::ID_Type>					id_mapping;

		util::vector<ID::Generation_Type>			generations;
		util::deque<ScriptID>						free_ids;

		util::vector<Transform::ComponentCache>		transform_cache;
#if USE_TRANSFORM_CACHE_MAP
		std::unordered_map<ID::ID_Type, u32>		cache_map;
#endif
		using ScriptRegistry = std::unordered_map<size_t, Detail::ScriptCreator>;
		ScriptRegistry& Registry() {
			// NOTE: Static variable in function to avoid any weird initialization bugs
			//			as order of initialization is unknown. This avoids that.
			static ScriptRegistry script_registry_internal;
			return script_registry_internal;
		}

#ifdef USE_WITH_EDITOR
		util::vector<std::string>& ScriptNames() {
			static util::vector<std::string> names;
			return names;
		}
#endif
	
		bool exists(ScriptID id) {
			assert(ID::IsValid(id));
			const ID::ID_Type index{ ID::Index(id) };
			assert(index < generations.size() && id_mapping[index] < entity_scripts.size());
			assert(generations[index] == ID::Generation(id));
			return (generations[index] == ID::Generation(id)) &&
				entity_scripts[id_mapping[index]] &&
				entity_scripts[id_mapping[index]]->IsValid();
		}
#if USE_TRANSFORM_CACHE_MAP
		Transform::ComponentCache* const GetCachePtr(const GameEntity::Entity* const entity) {
			assert(GameEntity::IsAlive((*entity).GetID()));
			const Transform::TransformID id{ (*entity).Transform().GetID() };

			u32 index{ u32_invalid_id };
			auto pair = cache_map.try_emplace(id, ID::Invalid_ID);

			// cache_map didn't have an entry for this id, insert new entry
			if (pair.second) {
				index = (u32)transform_cache.size();
				transform_cache.emplace_back();
				transform_cache.back().id = id;
				cache_map[id] = index;
			}
			else index = cache_map[id];

			assert(index < transform_cache.size());
			return &transform_cache[index];
		}
#else 
		Transform::ComponentCache* const GetCachePtr(const GameEntity::Entity* const entity) {
			assert(GameEntity::IsAlive((*entity).GetID()));
			const Transform::TransformID id{ (*entity).Transform().GetID() };

			for (auto& cache : transform_cache) if (cache.id == id) return &cache;

			transform_cache.emplace_back();
			transform_cache.back().id = id;

			return &transform_cache.back();
		}
#endif

	}

	namespace Detail {
		u8 RegisterScript(size_t tag, ScriptCreator creator) {
			bool res{ Registry().insert(ScriptRegistry::value_type{tag, creator}).second };
			assert(res);
			return res;
		}

		ScriptCreator GetScriptCreatorInternal(size_t tag) {
			auto script = Zetta::Script::Registry().find(tag);
			assert(script != Zetta::Script::Registry().end() && script->first == tag);
			return script->second;
		}

#ifdef USE_WITH_EDITOR
		u8 AddScriptName(const char* name) {
			ScriptNames().emplace_back(name);
			return true;
		}
#endif
	}

	Component CreateScript(const InitInfo& info, GameEntity::Entity entity) {
		assert(entity.IsValid());
		assert(info.script_creator);

		ScriptID id{};
		if (free_ids.size() > ID::Minimum_Deleted_Elements) {
			id = free_ids.front();
			assert(!exists(id));
			free_ids.pop_front();
			id = ScriptID{ ID::NewGeneration(id) };
			++generations[ID::Index(id)];
		}
		else {
			id = ScriptID{ (ID::ID_Type)id_mapping.size() };
			id_mapping.emplace_back();
			generations.push_back(0);
		}

		assert(ID::IsValid(id));
		const ID::ID_Type index{ (ID::ID_Type)entity_scripts.size() };
		entity_scripts.emplace_back(info.script_creator(entity));
		assert(entity_scripts.back()->GetID() == entity.GetID());
		id_mapping[ID::Index(id)] = index;
		return Component{ id };
	}

	void RemoveScript(Component c) {
		assert(c.IsValid() && exists(c.GetID()));
		const ScriptID id{ c.GetID() };
		const ID::ID_Type index{ id_mapping[ID::Index(id)] };
		const ScriptID last_id{ entity_scripts.back()->Script().GetID() };
		util::EraseUnordered(entity_scripts, index);
		id_mapping[ID::Index(last_id)] = index;
		id_mapping[ID::Index(id)] = ID::Invalid_ID;
	}

	void Update(float dt) {
		for (auto& ptr : entity_scripts) ptr->Update(dt);
		if (transform_cache.size()) {
			Transform::Update(transform_cache.data(), (u32)transform_cache.size());
			transform_cache.clear();

#if USE_TRANSFORM_CACHE_MAP
			cache_map.clear();
#endif
		}
	}

	void EntityScript::SetRotation(const GameEntity::Entity* const entity, Math::v4 rotation_quaternion) {
		Transform::ComponentCache& cache{ *GetCachePtr(entity) };
		cache.flags |= Transform::ComponentFlags::Rotation;
		cache.rotation = rotation_quaternion;
	}

	void EntityScript::SetOrientation(const GameEntity::Entity* const entity, Math::v3 orientation_vector) {
		Transform::ComponentCache& cache{ *GetCachePtr(entity) };
		cache.flags |= Transform::ComponentFlags::Orientation;
		cache.orientation = orientation_vector;
	}

	void EntityScript::SetPosition(const GameEntity::Entity* const entity, Math::v3 position) {
		Transform::ComponentCache& cache{ *GetCachePtr(entity) };
		cache.flags |= Transform::ComponentFlags::Position;
		cache.position = position;
	}

	void EntityScript::SetScale(const GameEntity::Entity* const entity, Math::v3 scale) {
		Transform::ComponentCache& cache{ *GetCachePtr(entity) };
		cache.flags |= Transform::ComponentFlags::Scale;
		cache.scale = scale;
	}

}

#ifdef USE_WITH_EDITOR
#include <atlsafe.h>

	extern "C" __declspec(dllexport)
	LPSAFEARRAY GetScriptNamesInternal() {
		const u32 size{ (u32)Zetta::Script::ScriptNames().size() };
		if (!size) return nullptr;
		CComSafeArray<BSTR> names(size);
		for (u32 i{ 0 }; i < size; i++)
			names.SetAt(i, A2BSTR_EX(Zetta::Script::ScriptNames()[i].c_str()), false);
		return names.Detach();
	}

#endif