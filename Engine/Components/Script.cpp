#include "Script.h"
#include "Entity.h"

namespace Zetta::Script {
	namespace {
		util::vector<Detail::ScriptPtr> entity_scripts;
		util::vector<ID::ID_Type> id_mapping;

		util::vector<ID::Generation_Type> generations;
		util::deque<ScriptID> free_ids;

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