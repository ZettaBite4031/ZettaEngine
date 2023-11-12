#pragma once

#include "Test.h"
#include "../Engine/Components/Entity.h"
#include "../Engine/Components/Transform.h"

#include <iostream>
#include <ctime>

using namespace Zetta;

class EntityComponentTest : public Test {
public:
	bool Initialize() override { 
		srand((u32)time(nullptr));
		return true; 
	}
	
	void Run() override {
		do {
			for (u32 i{ 0 }; i < 10000; i++) {
				CreateRandom();
				RemoveRandom();
				_num_entities = (u32)_entities.size();
			}
			PrintResults();
		} while (getchar() != 'q');
	}

	void Shutdown() override { }

private:

	void CreateRandom() {
		u32 count = rand() % 20;
		if (_entities.empty())count = 1000;
		Transform::InitInfo transform_info{};

		GameEntity::EntityInfo entity_info{
			&transform_info
		};

		while (count > 0) {
			++_added;
			GameEntity::Entity entity{ GameEntity::CreateGameEntity(entity_info) };
			assert(entity.IsValid());
			_entities.push_back(entity);
			assert(GameEntity::IsAlive(entity.GetID()));
			count--;
		}
	}

	void RemoveRandom() {
		u32 count = rand() % 20;
		if (_entities.size() < 1000) return;
		while (count > 0) {
			_removed++;
			const u32 index{ (u32)rand() % (u32)_entities.size() };
			const GameEntity::Entity entity{ _entities[index] };
			assert(entity.IsValid());
			if (entity.IsValid()) {
				GameEntity::RemoveGameEntity(entity.GetID());
				_entities.erase(_entities.begin() + index);
				assert(!GameEntity::IsAlive(entity.GetID()));
			}
			count--;
		}
	}

	void PrintResults() {
		std::cout << "Entities Created: " << _added << "\n";
		std::cout << "Entities Remvoed: " << _removed << "\n";
	}

	util::vector<GameEntity::Entity> _entities;
	u32 _added{ 0 };
	u32 _removed{ 0 };
	u32 _num_entities{ 0 };
};