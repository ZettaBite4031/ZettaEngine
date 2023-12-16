#pragma once

#include "../Components/ComponentCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

namespace Zetta{
	namespace GameEntity {
		DEFINE_TYPED_ID(EntityID);

		class Entity {
		public:
			constexpr explicit Entity(EntityID id) : _id{ id } {}
			constexpr Entity() : _id{ ID::Invalid_ID } {}
			constexpr EntityID GetID() const { return _id; }
			constexpr bool IsValid() const { return ID::IsValid(_id); }

			[[nodiscard]] Transform::Component Transform() const;
			[[nodiscard]] Script::Component Script() const;

			[[nodiscard]] Math::v4 Rotation() const { return Transform().Rotation(); }
			[[nodiscard]] Math::v3 Orientation() const { return Transform().Orientation(); }
			[[nodiscard]] Math::v3 Position() const { return Transform().Position(); }
			[[nodiscard]] Math::v3 Scale() const { return Transform().Scale(); }

		private:
			EntityID _id;
		};
	}

	namespace Script {
		class EntityScript : public GameEntity::Entity {
		public:
			virtual ~EntityScript() = default;
			virtual void OnWake() {}
			virtual void Update(float) {}
		protected:
			constexpr explicit EntityScript(GameEntity::Entity entity)
				: GameEntity::Entity{ entity.GetID() } { }

			void SetRotation(Math::v4 rotation_quaternion) const { SetRotation(this, rotation_quaternion); }
			void SetOrientation(Math::v3 orientation_vector) const { SetOrientation(this, orientation_vector); }
			void SetPosition(Math::v3 position) const { SetPosition(this, position); }
			void SetScale(Math::v3 scale) const { SetScale(this, scale); }

			static void SetRotation(const GameEntity::Entity* const entity, Math::v4 rotation_quaternion);
			static void SetOrientation(const GameEntity::Entity* const entity, Math::v3 orientation_vector);
			static void SetPosition(const GameEntity::Entity* const entity, Math::v3 position);
			static void SetScale(const GameEntity::Entity* const entity, Math::v3 scale);
		};

		namespace Detail {
			using ScriptPtr = std::unique_ptr<EntityScript>;
			using ScriptCreator = ScriptPtr(*)(GameEntity::Entity entity);
			using StringHash = std::hash<std::string>;

			u8 RegisterScript(size_t, ScriptCreator);

#ifdef USE_WITH_EDITOR
			extern "C" __declspec(dllexport)
#endif
				ScriptCreator GetScriptCreatorInternal(size_t);

			template<class ScriptClass>
			ScriptPtr CreateScript(GameEntity::Entity entity) {
				assert(entity.IsValid());
				return std::make_unique<ScriptClass>(entity);
			}
#ifdef USE_WITH_EDITOR
			u8 AddScriptName(const char* name);


#define REGISTER_SCRIPT(TYPE)																					\
		namespace {																								\
			const u8 _reg_##TYPE{ Zetta::Script::Detail::RegisterScript(										\
				Zetta::Script::Detail::StringHash()(#TYPE),														\
				&Zetta::Script::Detail::CreateScript<TYPE>) };													\
			const u8 _name_##TYPE{ Zetta::Script::Detail::AddScriptName(#TYPE) };								\
		}
#else
#define REGISTER_SCRIPT(TYPE)																					\
		namespace {																								\
			const u8 _reg_##TYPE{ Zetta::Script::Detail::RegisterScript(										\
				Zetta::Script::Detail::StringHash()(#TYPE), &Zetta::Script::Detail::CreateScript<TYPE>) };		\
																												\
		}						
#endif
		}
	}
}