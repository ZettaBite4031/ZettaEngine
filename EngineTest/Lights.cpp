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
	util::vector<Graphics::Light> disabled_lights;

	constexpr Math::v3 RGBtoColor(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }

	f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

	void CreateLight(Math::v3 position, Math::v3 rotation, Graphics::Light::Type type, u64 light_set_key) {
		const char* script_name{ nullptr }; // { type == Graphics::Light::Spot ? "RotatorScript" : nullptr };
		GameEntity::EntityID entity_id{ CreateGameEntity(position, rotation, script_name).GetID() };

		Graphics::LightInitInfo info{};
		info.entity_id = entity_id;
		info.type = type;
		info.light_set_key = light_set_key;
		info.intensity = 10.f;

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
	Graphics::CreateLightSet(left_set);
	Graphics::CreateLightSet(right_set);
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

	constexpr f32 scale1{ 2 };
	constexpr Math::v3 scale{ 1.f * scale1, 0.5f * scale1, 1.f * scale1 };
	constexpr s32 dim{ 8 };
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

	for (auto& light : disabled_lights) {
		const GameEntity::EntityID id{ light.EntityID() };
		Graphics::RemoveLight(light.GetID(), light.GetLightSetKey());
		RemoveGameEntity(id);
	}
	disabled_lights.clear();

	Graphics::RemoveLightSet(left_set);
	Graphics::RemoveLightSet(right_set);
}

void TestLights(f32 dt) {
#if 0
	static f32 t{ 0 };
	t += 0.05f;
	for (u32 i{ 0 }; i < (u32)lights.size(); i++) {
		f32 sine{ DirectX::XMScalarSin(t + lights[i].GetID()) };
		sine *= sine;
		lights[i].Intensity(2.f * sine);
	}
#elif 1
	u32 count{ (u32)(random(0.1f) * 100) };
	for (u32 i{ 0 }; i < count; i++) {
		if (!lights.size()) break;
		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		Graphics::Light light{ lights[index] };
		light.IsEnabled(false);
		util::EraseUnordered(lights, index);
		disabled_lights.emplace_back(light);
	}

	count = (u32)(random(0.1f) * 50);
	for (u32 i{ 0 }; i < count; i++) {
		if (!lights.size()) break;
		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		Graphics::Light light{ lights[index] };
		const GameEntity::EntityID id{ light.EntityID() };
		Graphics::RemoveLight(light.GetID(), light.GetLightSetKey());
		RemoveGameEntity(id);
		util::EraseUnordered(lights, index);
	}

	count = (u32)(random(0.1f) * 50);
	for (u32 i{ 0 }; i < count; i++) {
		if (!disabled_lights.size()) break;
		const u32 index{ (u32)(random() * (disabled_lights.size() - 1)) };
		Graphics::Light light{ disabled_lights[index] };
		const GameEntity::EntityID id{ light.EntityID() };
		Graphics::RemoveLight(light.GetID(), light.GetLightSetKey());
		RemoveGameEntity(id);
		util::EraseUnordered(disabled_lights, index);
	}

	count = (u32)(random(0.1f) * 100);
	for (u32 i{ 0 }; i < count; i++) {
		if (!disabled_lights.size()) break;
		const u32 index{ (u32)(random() * (disabled_lights.size() - 1)) };
		Graphics::Light light{ disabled_lights[index] };
		light.IsEnabled(true);
		util::EraseUnordered(disabled_lights, index);
		lights.emplace_back(light);
	}

	constexpr f32 scale1{ 1 };
	constexpr Math::v3 scale{ 1.f * scale1, 0.5f * scale1, 1.f * scale1 };
	count = (u32)(random(0.1f) * 50);
	for (u32 i{ 0 }; i < count; i++) {
		Math::v3 p1{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		Math::v3 p2{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		CreateLight(p1, { random() * 3.14f, random() * 3.14f, random() * 3.14f },
			random() > 0.5f ? Graphics::Light::Spot : Graphics::Light::Point, left_set);
		CreateLight(p2, { random() * 3.14f, random() * 3.14f, random() * 3.14f },
			random() > 0.5f ? Graphics::Light::Spot : Graphics::Light::Point, right_set);
	}
#endif
}