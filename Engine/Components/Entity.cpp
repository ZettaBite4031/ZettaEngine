#include "Entity.h"
#include "Transform.h"
#include "Script.h"

namespace Zetta::GameEntity {
	namespace {
		util::vector<Transform::Component>	transforms;
		util::vector<Script::Component>	scripts;

		util::vector<ID::Generation_Type>	generations;
		util::deque<EntityID>				free_ids;
	}

	Entity CreateGameEntity(const EntityInfo& info) {
		assert(info.transform); // All game entities must have a transform
		if (!info.transform) return Entity{};

		EntityID id;

		if (free_ids.size() > ID::Minimum_Deleted_Elements) {
			id = free_ids.front();
			assert(!IsAlive(id));
			free_ids.pop_front();
			id = EntityID{ ID::NewGeneration(id) };
			++generations[ID::Index(id)];
		}
		else {
			id = EntityID{ (ID::ID_Type)generations.size() };
			generations.push_back(0);
			transforms.emplace_back();
			scripts.emplace_back();
		}

		const Entity newEntity{ id };
		const ID::ID_Type index{ ID::Index(id) };

		assert(!transforms[index].IsValid());
		transforms[index] = Transform::CreateTransform(*info.transform, newEntity);
		if (!transforms[index].IsValid()) return {};

		if (info.script && info.script->script_creator) {
			assert(!scripts[index].IsValid());
			scripts[index] = Script::CreateScript(*info.script, newEntity);
			assert(scripts[index].IsValid());
		}

		return newEntity;
	}

	void RemoveGameEntity(EntityID id) {
		const ID::ID_Type index{ ID::Index(id) };
		assert(IsAlive(id));

		if (scripts[index].IsValid()) {
			Script::RemoveScript(scripts[index]);
			scripts[index] = {};
		}

		Transform::RemoveTransform(transforms[index]);
		transforms[index] = {};
		free_ids.push_back(id);
		
	}

	bool IsAlive(EntityID id) {
		assert(ID::IsValid(id));
		const ID::ID_Type index{ ID::Index(id) };
		assert(index < generations.size());
		return (generations[index] == ID::Generation(id) && transforms[index].IsValid());
	}

	Transform::Component Entity::Transform() const {
		assert(IsAlive(_id));
		const ID::ID_Type index{ ID::Index(_id) };
		return transforms[index];
	}

	Script::Component Entity::Script() const{
		assert(IsAlive(_id));
		const ID::ID_Type index{ ID::Index(_id) };
		return scripts[index];
	}
}