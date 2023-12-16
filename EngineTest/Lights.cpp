#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"

#define RANDOM_LIGHTS 1

using namespace Zetta;

GameEntity::Entity CreateGameEntity(Math::v3 position, Math::v3 rotation, const char* script_name);
void RemoveGameEntity(GameEntity::EntityID id);

namespace {
	const u64 left_set{ 0 };
	const u64 right_set{ 1 };
	constexpr f32 inv_rand_max{ 1.f / RAND_MAX };

	util::vector<Graphics::Light> lights;

	constexpr Math::v3 RGBtoColor(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }

	f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

	void CreateLight(Math::v3 position, Math::v3 rotation, Graphics::Light::Type type, u64 light_set_key) {
		GameEntity::EntityID entity_id{ CreateGameEntity(position, rotation, nullptr).GetID() };

		Graphics::LightInitInfo info{};
		info.entity_id = entity_id;
		info.type = type;
		info.light_set_key = light_set_key;
		info.intensity = 1.f;

		info.color = { random(0.2f), random(0.2f), random(0.2f) };

#if RANDOM_LIGHTS
		if (type == Graphics::Light::Point) {
			info.point_params.range = random(0.5f) * 2.f;
			info.point_params.attenuation = { 1,1,1 };
		}

		else if (type == Graphics::Light::Spot) {
			info.spot_params.range = random(0.5f) * 2.f;
			info.spot_params.umbra = (random(0.5f) - 0.4f) * Math::PI;
			info.spot_params.penumbra = info.spot_params.umbra + (0.1f * Math::PI);
			info.spot_params.attenuation = { 1,1,1 };
		}
#else
		if (type == Graphics::Light::Point) {
			info.point_params.range = 1.f;
			info.point_params.attenuation = { 1,1,1 };
		}

		else if (type == Graphics::Light::Spot) {
			info.spot_params.range = 2.f;
			info.spot_params.umbra = 0.1f * Math::PI;
			info.spot_params.penumbra = info.spot_params.umbra + (0.1f * Math::PI);
			info.spot_params.attenuation = { 1,1,1 };
		}
#endif

		Graphics::Light light{ Graphics::CreateLight(info) };
		assert(light.IsValid());
		lights.push_back(light);
	}
}

	void GenerateLights() {
		// LEFT SET
		Graphics::LightInitInfo info{};
		info.entity_id = CreateGameEntity({}, { 0, 0, 0 }, nullptr).GetID();
		info.type = Graphics::Light::Directional;
		info.light_set_key = left_set;
		info.intensity = 1.f;
		info.color = RGBtoColor(174, 174, 174);
		lights.emplace_back(Graphics::CreateLight(info));

		info.entity_id = CreateGameEntity({}, { Math::PI * 0.5f, 0, 0 }, nullptr).GetID();
		info.color = RGBtoColor(17, 27, 48);
		lights.emplace_back(Graphics::CreateLight(info));
		
		info.entity_id = CreateGameEntity({}, { -Math::PI * 0.5f, 0, 0 }, nullptr).GetID();
		info.color = RGBtoColor(63, 47, 30);
		lights.emplace_back(Graphics::CreateLight(info));

		// RIGHT SET
		info.entity_id = CreateGameEntity({}, { 0, 0, 0 }, nullptr).GetID();
		info.light_set_key = right_set;
		info.color = RGBtoColor(150, 100, 200);
		lights.emplace_back(Graphics::CreateLight(info));

		info.entity_id = CreateGameEntity({}, { Math::PI * 0.5f, 0, 0 }, nullptr).GetID();
		info.color = RGBtoColor(17, 27, 48);
		lights.emplace_back(Graphics::CreateLight(info));

		info.entity_id = CreateGameEntity({}, { -Math::PI * 0.5f, 0, 0 }, nullptr).GetID();
		info.color = RGBtoColor(63, 47, 30);
		lights.emplace_back(Graphics::CreateLight(info));

#if !RANDOM_LIGHTS

		CreateLight({ 0, -3, 0 }, {}, Graphics::Light::Point, left_set);
		CreateLight({ 0, 0.2f, 1.f }, {}, Graphics::Light::Point, left_set);
		CreateLight({ 0, 3, 2.5f }, {}, Graphics::Light::Point, left_set);
		CreateLight({ 0, 0.1f, 7.f }, {0, 3.14f, 0}, Graphics::Light::Spot, left_set);
#else
		srand(37);

		constexpr f32 scale1{ 3 };
		constexpr Math::v3 scale{ 1.f * scale1, 0.5f * scale1, 1.f * scale1 };
		constexpr s32 dim{ 10 };
		for(s32 x{-dim}; x < dim; x++)
			for(s32 y{ 0 }; y < 2 * dim; y++)
				for (s32 z{ -dim }; z < dim; z++) {
					CreateLight({ (f32)(x * scale.x) , (f32)(y * scale.y), (f32)(z * scale.z) },
						{ 3.14f, random(), 0.f }, random() > 0.5f ? Graphics::Light::Spot : Graphics::Light::Point, left_set);
					CreateLight({ (f32)(x * scale.x) , (f32)(y * scale.y), (f32)(z * scale.z) },
						{ 3.14f, random(), 0.f }, random() > 0.5f ? Graphics::Light::Spot : Graphics::Light::Point, right_set);
				}
#endif
	}
	
	void RemoveLights() {
		for (auto& light : lights) {
			const GameEntity::EntityID id{ light.EntityID() };
			Graphics::RemoveLight(light.GetID(), light.GetLightSetKey());
			RemoveGameEntity(id);
		}
		lights.clear();
	}