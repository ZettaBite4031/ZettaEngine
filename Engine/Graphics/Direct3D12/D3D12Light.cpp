#include "D3D12Core.h"
#include "D3D12Light.h"
#include "Shaders/SharedTypes.h"
#include "EngineAPI/GameEntity.h"
#include "Components/Transform.h"

namespace Zetta::Graphics::D3D12::Light {
	namespace {
		template<u32 n>
		struct u32_set_bits {
			static_assert(n > 0 && n <= 32);
			constexpr static const u32 bits{ u32_set_bits<n - 1>::bits | (1 << (n - 1)) };
		};

		template<>
		struct u32_set_bits<0> {
			constexpr static const u32 bits{ 0 };
		};

		static_assert((u32_set_bits<FrameBufferCount>::bits < (1 << 8), "That's quite a large buffer count!"));

		constexpr u32 dirty_bits_mask{ (u8)u32_set_bits<FrameBufferCount>::bits };

		struct LightOwner {
			GameEntity::EntityID entity_id{ ID::Invalid_ID };
			u32 data_index{ u32_invalid_id };
			Graphics::Light::Type type;
			bool is_enabled;
		};
#if USE_STL_VECTOR
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif
		class LightSet {
		public:
			constexpr Graphics::Light Add(const LightInitInfo& info) {
				if (info.type == Graphics::Light::Directional)
				{
					u32 index{ u32_invalid_id };
					// Find an available slot in the array if any.
					for (u32 i{ 0 }; i < _non_cullable_owners.size(); ++i)
					{
						if (!ID::IsValid(_non_cullable_owners[i]))
						{
							index = i;
							break;
						}
					}

					if (index == u32_invalid_id)
					{
						index = (u32)_non_cullable_owners.size();
						_non_cullable_owners.emplace_back();
						_non_cullable_lights.emplace_back();
					}

					HLSL::DirectionalLightParameters& params{ _non_cullable_lights[index] };
					params.Color = info.color;
					params.Intensity = info.intensity;

					LightOwner owner{ GameEntity::EntityID{info.entity_id}, index, info.type, info.is_enabled };
					const LightID id{ _owners.Add(owner) };
					_non_cullable_owners[index] = id;

					return Graphics::Light{ id, info.light_set_key };
				}
				else
				{
					u32 index{ u32_invalid_id };

					// Try to find an empty slot
					for (u32 i{ _enabled_light_count }; i < _cullable_owners.size(); ++i)
					{
						if (!ID::IsValid(_cullable_owners[i]))
						{
							index = i;
							break;
						}
					}

					// If no empty slot was found then add a new item
					if (index == u32_invalid_id)
					{
						index = (u32)_cullable_owners.size();
						_cullable_lights.emplace_back();
						_culling_info.emplace_back();
						_cullable_entity_ids.emplace_back();
						_cullable_owners.emplace_back();
						_dirty_bits.emplace_back();
						assert(_cullable_owners.size() == _cullable_lights.size());
						assert(_cullable_owners.size() == _culling_info.size());
						assert(_cullable_owners.size() == _cullable_entity_ids.size());
						assert(_cullable_owners.size() == _dirty_bits.size());
					}

					AddCullableLightParameters(info, index);
					AddLightCullingInfo(info, index);
					const LightID id{ _owners.Add(LightOwner{GameEntity::EntityID{info.entity_id}, index, info.type, info.is_enabled}) };
					_cullable_entity_ids[index] = _owners[id].entity_id;
					_cullable_owners[index] = id;
					_dirty_bits[index] = dirty_bits_mask;
					Enable(id, info.is_enabled);
					UpdateTransforms(index);

					return Graphics::Light{ id, info.light_set_key };
				}
			}

			constexpr void Remove(LightID id) {
				Enable(id, false);

				const LightOwner& owner{ _owners[id] };
				if (owner.type == Graphics::Light::Directional) {
					_non_cullable_owners[owner.data_index] = LightID{ ID::Invalid_ID };
				}
				else {
					auto test2 = _cullable_owners[owner.data_index];
					auto test = _owners[_cullable_owners[owner.data_index]];
					assert(_owners[_cullable_owners[owner.data_index]].data_index == owner.data_index);
					_cullable_owners[owner.data_index] = LightID{ ID::Invalid_ID };
				}
				_owners.Remove(id);
			}

			void UpdateTransforms() {
				for (const auto& id : _non_cullable_owners) {
					if (!ID::IsValid(id)) continue;

					const LightOwner& owner{ _owners[id] };
					if (owner.is_enabled) {
						const GameEntity::Entity entity{ GameEntity::EntityID{owner.entity_id} };
						HLSL::DirectionalLightParameters& params{ _non_cullable_lights[owner.data_index] };
						params.Direction = entity.Orientation();
					}
				}

				// Update position and direction of enabled cullable lights

				const u32 count{ _enabled_light_count };
				if (!count) return;
				
				assert(_cullable_entity_ids.size() >= count);
				transform_flags_cache.resize(count);
				Transform::GetUpdatedComponentFlags(_cullable_entity_ids.data(), count, transform_flags_cache.data());
				
				for (u32 i{ 0 }; i < count; i++) 
					if (transform_flags_cache[i]) UpdateTransforms(i);
				
			}

			constexpr void Enable(LightID id, bool is_enabled) {
				_owners[id].is_enabled = is_enabled;

				if (_owners[id].type == Graphics::Light::Directional)
				{
					return;
				}

				// Cullable lights
				const u32 data_index{ _owners[id].data_index };

				// NOTE: this is a reference to _enabled_light_count and will change its value!
				u32& count{ _enabled_light_count };

				// NOTE: ditry_bits is going to be set by swap_cullable_lightsm so we don't set it here.
				if (is_enabled)
				{
					if (data_index > count)
					{
						assert(count < _cullable_lights.size());
						SwapCullableLights(data_index, count);
						++count;
					}
					else if (data_index == count)
					{
						++count;
					}
				}
				else if (count > 0)
				{
					const u32 last{ count - 1 };
					if (data_index < last)
					{
						SwapCullableLights(data_index, last);
						--count;
					}
					else if (data_index == last)
					{
						--count;
					}
				}
			}

			constexpr void Intensity(LightID id, f32 intensity) {
				if (intensity < 0.f) intensity = 0.f;

				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == Graphics::Light::Directional) {
					assert(index < _non_cullable_lights.size());
					_non_cullable_lights[index].Intensity = intensity;
				}
				else {
					assert(_owners[_cullable_owners[index]].data_index == index);
					assert(index < _cullable_lights.size());
					_cullable_lights[index].Intensity = intensity;
					_dirty_bits[index] = dirty_bits_mask;
				}
			}

			constexpr void Color(LightID id, Math::v3 color) {
				assert(color.x <= 1.f && color.y <= 1.f && color.z <= 1.f);
				assert(color.x >= 0.f && color.y >= 0.f && color.z >= 0.f);

				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == Graphics::Light::Directional) {
					assert(index < _non_cullable_lights.size());
					_non_cullable_lights[index].Color = color;
				}
				else {
					assert(_owners[_cullable_owners[index]].data_index == index);
					assert(index < _cullable_lights.size());
					_cullable_lights[index].Color = color;
					_dirty_bits[index] = dirty_bits_mask;
				}
			}

			CONSTEXPR void Attenuation(LightID id, Math::v3 attenuation) {
				assert(attenuation.x >= 0.f && attenuation.y >= 0.f && attenuation.z >= 0.f);
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				_cullable_lights[index].Attenuation = attenuation;
				_dirty_bits[index] = dirty_bits_mask;
			}

			CONSTEXPR void Range(LightID id, f32 range) {
				assert(range >= 0.f);
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				_cullable_lights[index].Range = range;
				_culling_info[index].Range = range;
				_dirty_bits[index] = dirty_bits_mask;

				if (owner.type == Graphics::Light::Spot) 
					_culling_info[index].ConeRadius = CalculateConeRadius(range, _cullable_lights[index].CosPenumbra);
				
			}

			void Umbra(LightID id, f32 umbra) {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				umbra = Math::clamp(umbra, 0.f, Math::PI);
				_cullable_lights[index].CosUmbra = DirectX::XMScalarCos(umbra * 0.5f);
				_dirty_bits[index] = dirty_bits_mask;
				if (Penumbra(id) < umbra) Penumbra(id, umbra);
			}

			void Penumbra(LightID id, f32 penumbra) {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				penumbra = Math::clamp(penumbra, Umbra(id), Math::PI);
				_cullable_lights[index].CosPenumbra = DirectX::XMScalarCos(penumbra * 0.5f);
				_culling_info[index].ConeRadius = CalculateConeRadius(Range(id), _cullable_lights[index].CosPenumbra);
				_dirty_bits[index] = dirty_bits_mask;
			}

			constexpr bool IsEnabled(LightID id) const {
				return _owners[id].is_enabled;
			}

			constexpr f32 Intensity(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == Graphics::Light::Directional) {
					assert(index < _non_cullable_lights.size());
					return _non_cullable_lights[index].Intensity;
				}
				
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].Intensity;
			}

			constexpr Math::v3 Color(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == Graphics::Light::Directional) {
					assert(index < _non_cullable_lights.size());
					return _non_cullable_lights[index].Color;
				}
				
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].Color;
			}

			CONSTEXPR Math::v3 Attenuation(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].Attenuation;
			}

			CONSTEXPR f32 Range(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != Graphics::Light::Directional);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].Range;
			}

			f32 Umbra(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == Graphics::Light::Spot);
				assert(index < _cullable_lights.size());
				return DirectX::XMScalarACos(_cullable_lights[index].CosUmbra) * 2.f;
			}

			f32 Penumbra(LightID id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == Graphics::Light::Spot);
				assert(index < _cullable_lights.size());
				return DirectX::XMScalarACos(_cullable_lights[index].CosPenumbra) * 2.f;
			}

			constexpr Graphics::Light::Type LightType(LightID id) const {
				return _owners[id].type;
			}

			constexpr ID::ID_Type EntityID(LightID id) const {
				return _owners[id].entity_id;
			}
			CONSTEXPR u32 NoncullableLightCount() const {
				u32 count{ 0 };
				for (const auto& id : _non_cullable_owners)
					if (ID::IsValid(id) && _owners[id].is_enabled) count++;
				return count;
			}

			CONSTEXPR void NoncullableLights(HLSL::DirectionalLightParameters* const lights, [[maybe_unused]] u32 size) {
				assert(size == Math::AlignSizeUp<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(NoncullableLightCount() * sizeof(HLSL::DirectionalLightParameters)));
				const u32 count{ (u32)_non_cullable_owners.size() };
				u32 index{ 0 };
				for (u32 i{ 0 }; i < count; i++) {
					if (!ID::IsValid(_non_cullable_owners[i])) continue;

					const LightOwner& owner{ _owners[_non_cullable_owners[i]] };
					if (owner.is_enabled) {
						assert(_owners[_non_cullable_owners[i]].data_index == i);
						lights[index] = _non_cullable_lights[i];
						index++;
					}
				}
			}

			constexpr u32 CullableLightCount() const {
				return _enabled_light_count;
			}

			constexpr bool HasLights() const {
				return _owners.size() > 0;
			}

		private:
			f32 CalculateConeRadius(f32 range, f32 cos_penumbra) {
				const f32 sin_penumbra{ sqrt(1.f - cos_penumbra * cos_penumbra) };
				return sin_penumbra * range;
			}

			void UpdateTransforms(u32 index) {
				const GameEntity::Entity entity{ GameEntity::EntityID{_cullable_entity_ids[index]} };
				HLSL::LightParameters& params{ _cullable_lights[index] };
				params.Position = entity.Position();

				HLSL::LightCullingLightInfo& culling_info{ _culling_info[index] };
				culling_info.Position = params.Position;

				if (params.Type == Graphics::Light::Spot) 
					culling_info.Direction = params.Direction = entity.Orientation();
				
				_dirty_bits[index] = dirty_bits_mask;
			}

			CONSTEXPR void AddCullableLightParameters(const LightInitInfo& info, u32 index) {
				using Graphics::Light;
				assert(info.type != Light::Directional && index < _cullable_lights.size());

				HLSL::LightParameters& params{ _cullable_lights[index] };
				params.Type = info.type;
				assert(params.Type < Light::count);
				params.Color = info.color;
				params.Intensity = params.Intensity;

				if (params.Type == Light::Point) {
					const PointLightParams& p{ info.point_params };
					params.Attenuation = p.attenuation;
					params.Range = p.range;
				}
				else if (params.Type == Light::Spot) {
					const SpotLightParams& p{ info.spot_params };
					params.Attenuation = p.attenuation;
					params.Range = p.range;
					params.CosUmbra = DirectX::XMScalarCos(p.umbra * 0.5f);
					params.CosPenumbra = DirectX::XMScalarCos(p.penumbra * 0.5f);
				}
			}

			CONSTEXPR void AddLightCullingInfo(const LightInitInfo& info, u32 index) {
				using Graphics::Light;
				assert(info.type != Light::Directional && index < _culling_info.size());
				
				HLSL::LightParameters& params{ _cullable_lights[index] };
				assert(params.Type == info.type);

				HLSL::LightCullingLightInfo& culling_info{ _culling_info[index] };
				culling_info.Range = params.Range;
				culling_info.Type = params.Type;

				if (info.type == Light::Spot) 
					culling_info.ConeRadius = CalculateConeRadius(params.Range, params.CosPenumbra);
			}

			void SwapCullableLights(u32 index1, u32 index2) {
				assert(index1 != index2);
				assert(index1 < _cullable_owners.size());
				assert(index2 < _cullable_owners.size());
				assert(index1 < _culling_info.size());
				assert(index2 < _culling_info.size());
				assert(index1 < _cullable_lights.size());
				assert(index2 < _cullable_lights.size());
				assert(index1 < _cullable_entity_ids.size());
				assert(index2 < _cullable_entity_ids.size());
				assert(ID::IsValid(_cullable_owners[index1]) || ID::IsValid(_cullable_owners[index2]));

				if (!ID::IsValid(_cullable_owners[index2])) std::swap(index1, index2);
				if (!ID::IsValid(_cullable_owners[index1])) {
					LightOwner& owner2{ _owners[_cullable_owners[index2]] };
					assert(owner2.data_index = index2);
					owner2.data_index = index1;

					_culling_info[index1] = _culling_info[index2];
					_cullable_lights[index1] = _cullable_lights[index2];
					_cullable_entity_ids[index1] = _cullable_entity_ids[index2];
					std::swap(_cullable_owners[index1], _cullable_owners[index2]);
					_dirty_bits[index1] = dirty_bits_mask;
					assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
					assert(ID::IsValid(_cullable_owners[index1]));
				}
				else {
					// Swap light parameter indices
					LightOwner& owner1{ _owners[_cullable_owners[index1]] };
					LightOwner& owner2{ _owners[_cullable_owners[index2]] };
					assert(owner1.data_index == index1);
					assert(owner2.data_index == index2);
					owner1.data_index = index2;
					owner2.data_index = index1;

					std::swap(_cullable_lights[index1], _cullable_lights[index2]);
					std::swap(_culling_info[index1], _culling_info[index2]);
					std::swap(_cullable_entity_ids[index1], _cullable_entity_ids[index2]);
					std::swap(_cullable_owners[index1], _cullable_owners[index2]);

					assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
					assert(_owners[_cullable_owners[index2]].entity_id == _cullable_entity_ids[index2]);

					// Set dirty bits
					assert(index1 < _dirty_bits.size());
					assert(index2 < _dirty_bits.size());
					_dirty_bits[index1] = dirty_bits_mask;
					_dirty_bits[index2] = dirty_bits_mask;
				}
			}

			// NOTE: These are NOT tightly packed
			util::FreeList<LightOwner>						_owners;
			util::vector<HLSL::DirectionalLightParameters>	_non_cullable_lights;
			util::vector<LightID>							_non_cullable_owners;

			// NOTE: These are tightly packed
			util::vector<HLSL::LightParameters>				_cullable_lights;
			util::vector<HLSL::LightCullingLightInfo>		_culling_info;
			util::vector<GameEntity::EntityID>				_cullable_entity_ids;
			util::vector<LightID>							_cullable_owners;
			util::vector<u8>								_dirty_bits;
				
			util::vector<u8>								transform_flags_cache;
			u32												_enabled_light_count{ 0 };

			friend class D3D12LightBuffer;
		};

		class D3D12LightBuffer {
		public:
			D3D12LightBuffer() = default;
			CONSTEXPR void UpdateLightBuffers(LightSet& set, u64 light_set_key, u32 frame_idx) {
				u32 sizes[LightBuffer::count]{};
				sizes[LightBuffer::NoncullableLights] = set.NoncullableLightCount() * sizeof(HLSL::DirectionalLightParameters);
				sizes[LightBuffer::CullableLights] = set.CullableLightCount() * sizeof(HLSL::LightParameters);
				sizes[LightBuffer::CullingInfo] = set.CullableLightCount() * sizeof(HLSL::LightCullingLightInfo);

				u32 current_sizes[LightBuffer::count]{};
				current_sizes[LightBuffer::NoncullableLights] = _buffers[LightBuffer::NoncullableLights].buffer.Size();
				current_sizes[LightBuffer::CullableLights] = _buffers[LightBuffer::CullableLights].buffer.Size();
				current_sizes[LightBuffer::CullingInfo] = _buffers[LightBuffer::CullingInfo].buffer.Size();

				if (current_sizes[LightBuffer::NoncullableLights] < sizes[LightBuffer::NoncullableLights]) {
					ResizeBuffer(LightBuffer::NoncullableLights, sizes[LightBuffer::NoncullableLights], frame_idx);
				}

				set.NoncullableLights((HLSL::DirectionalLightParameters* const)_buffers[LightBuffer::NoncullableLights].cpu_addr, 
					_buffers[LightBuffer::NoncullableLights].buffer.Size());

				// Update cullable light buffers
				bool buffers_resized{ false };
				if (current_sizes[LightBuffer::CullableLights] < sizes[LightBuffer::CullableLights]) {
					assert(current_sizes[LightBuffer::CullableLights] < sizes[LightBuffer::CullableLights]);
					ResizeBuffer(LightBuffer::CullableLights, sizes[LightBuffer::CullableLights], frame_idx);
					ResizeBuffer(LightBuffer::CullingInfo, sizes[LightBuffer::CullingInfo], frame_idx);
					buffers_resized = true;
				}

				bool all_lights_updated{ false };
				if (buffers_resized || _current_light_set_key != light_set_key) {
					memcpy(_buffers[LightBuffer::CullableLights].cpu_addr, set._cullable_lights.data(), sizes[LightBuffer::CullableLights]);
					memcpy(_buffers[LightBuffer::CullingInfo].cpu_addr, set._culling_info.data(), sizes[LightBuffer::CullingInfo]);
					_current_light_set_key = light_set_key;
					all_lights_updated = true;
				}

				assert(_current_light_set_key == light_set_key);
				const u32 index_mask{ 1UL << frame_idx };
				if (all_lights_updated) {
					for (u32 i{ 0 }; i < set.CullableLightCount(); i++) {
						set._dirty_bits[i] &= ~index_mask;
					}
				}
				else {
					for (u32 i{ 0 }; i < set.CullableLightCount(); i++) {
						if (set._dirty_bits[i] & index_mask) {
							assert(i * sizeof(HLSL::LightParameters) < sizes[LightBuffer::CullableLights]);
							assert(i * sizeof(HLSL::LightCullingLightInfo) < sizes[LightBuffer::CullingInfo]);
							u8* const light_dst{ _buffers[LightBuffer::CullableLights].cpu_addr + (i * sizeof(HLSL::LightParameters)) };
							u8* const culling_dst{ _buffers[LightBuffer::CullingInfo].cpu_addr + (i * sizeof(HLSL::LightCullingLightInfo)) };
							memcpy(light_dst, &set._cullable_lights[i], sizeof(HLSL::LightParameters));
							memcpy(culling_dst, &set._culling_info[i], sizeof(HLSL::LightCullingLightInfo));
							set._dirty_bits[i] &= ~index_mask;
						}
					}
				}
			}

			constexpr void Release() {
				for (u32 i{ 0 }; i < LightBuffer::count; i++) {
					_buffers[i].buffer.Release();
					_buffers[i].cpu_addr = nullptr;
				}
			}

			const D3D12_GPU_VIRTUAL_ADDRESS NoncullableLights() const { return _buffers[LightBuffer::NoncullableLights].buffer.GPU_Address(); }
			const D3D12_GPU_VIRTUAL_ADDRESS CullableLights() const { return _buffers[LightBuffer::CullableLights].buffer.GPU_Address(); }
			const D3D12_GPU_VIRTUAL_ADDRESS CullingInfo() const { return _buffers[LightBuffer::CullingInfo].buffer.GPU_Address(); }

		private:
			struct LightBuffer {
				enum Type : u32 {
					NoncullableLights,
					CullableLights,
					CullingInfo,

					count
				};

				D3D12Buffer buffer{};
				u8* cpu_addr{ nullptr };
			};

			void ResizeBuffer(LightBuffer::Type type, u32 size, [[maybe_unused]] u32 frame_idx) {
				assert(type < LightBuffer::count);
				if (!size) return;

				_buffers[type].buffer.Release();
				_buffers[type].buffer = D3D12Buffer{ ConstantBuffer::GetDefaultInitInfo(size), true };
				NAME_D3D12_OBJECT_INDEXED(_buffers[type].buffer.Buffer(), frame_idx,
					type == LightBuffer::NoncullableLights ? L"Noncullable Light Buffer" :
					type == LightBuffer::CullableLights ? L"Cullable Light Buffer" :
					type == LightBuffer::CullingInfo ? L"Culling Info Buffer" : L"Unknown Light Buffer");

				D3D12_RANGE range{};
				DXCall(_buffers[type].buffer.Buffer()->Map(0, &range, (void**)(&_buffers[type].cpu_addr)));
				assert(_buffers[type].cpu_addr);
			}

			LightBuffer _buffers[LightBuffer::count];
			u64 _current_light_set_key{ 0 };
		};


		std::unordered_map<u64, LightSet> light_sets;
		D3D12LightBuffer light_buffers[FrameBufferCount];

		constexpr void SetIsEnabled(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			bool is_enabled{ *(bool*)data };
			assert(sizeof(is_enabled) == size);
			set.Enable(id, is_enabled);
		}
		
		constexpr void SetIntensity(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			f32 intensity{ *(f32*)data };
			assert(sizeof(intensity) == size);
			set.Intensity(id, intensity);
		}

		constexpr void SetColor(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			Math::v3 color{ *(Math::v3*)data };
			assert(sizeof(color) == size);
			set.Color(id, color);
		}

		CONSTEXPR void SetAttenuation(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			Math::v3 attenuation{ *(Math::v3*)data };
			assert(sizeof(attenuation) == size);
			set.Attenuation(id, attenuation);
		}

		CONSTEXPR void SetRange(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			f32 range{ *(f32*)data };
			assert(sizeof(range) == size);
			set.Range(id, range);
		}

		void SetUmbra(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			f32 umbra{ *(f32*)data };
			assert(sizeof(umbra) == size);
			set.Umbra(id, umbra);
		}

		void SetPenumbra(LightSet& set, LightID id, const void* const data, [[maybe_unused]] u32 size) {
			f32 penumbra{ *(f32*)data };
			assert(sizeof(penumbra) == size);
			set.Penumbra(id, penumbra);
		}

		constexpr void GetIsEnabled(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size) {
			bool* const is_enabled{ (bool* const)data };
			assert(sizeof(bool) == size);
			*is_enabled = set.IsEnabled(id);
		}

		constexpr void GetIntensity(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size){
			f32* const intensity{ (f32* const)data };
			assert(sizeof(f32) == size);
			*intensity = set.Intensity(id);
		}

		constexpr void GetColor(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size){
			Math::v3* const color{ (Math::v3* const)data };
			assert(sizeof(Math::v3) == size);
			*color = set.Color(id);
		}

		CONSTEXPR void GetAttenuation(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size) {
			Math::v3* const attenuation{ (Math::v3* const)data };
			assert(sizeof(Math::v3) == size);
			*attenuation = set.Attenuation(id);
		}

		CONSTEXPR void GetRange(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size) {
			f32* const range{ (f32* const)data };
			assert(sizeof(Math::v3) == size);
			*range = set.Range(id);
		}

		void GetUmbra(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size) {
			f32* const umbra{ (f32* const)data };
			assert(sizeof(Math::v3) == size);
			*umbra = set.Umbra(id);
		}

		void GetPenumbra(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size) {
			f32* const penumbra{ (f32* const)data };
			assert(sizeof(Math::v3) == size);
			*penumbra = set.Penumbra(id);
		}

		constexpr void GetType(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size){
			Graphics::Light::Type* const type{ (Graphics::Light::Type* const)data };
			assert(sizeof(Graphics::Light::Type) == size);
			*type = set.LightType(id);
		}

		constexpr void GetEntityID(LightSet& set, LightID id, void* const data, [[maybe_unused]] u32 size){
			ID::ID_Type* const entity_id{ (ID::ID_Type* const)data };
			assert(sizeof(ID::ID_Type) == size);
			*entity_id = set.EntityID(id);
		}


		constexpr void DummySet(LightSet&, LightID, const void* const, u32) {}

		using SetFunction = void(*)(LightSet&, LightID, const void* const, u32);
		using GetFunction = void(*)(LightSet&, LightID, void* const, u32);

		constexpr SetFunction SetFunctions[]{
			SetIsEnabled,
			SetIntensity,
			SetColor,
			SetAttenuation,
			SetRange,
			SetUmbra,
			SetPenumbra,
			DummySet,
			DummySet,
		};

		static_assert(_countof(SetFunctions) == LightParameter::count);

		constexpr GetFunction GetFunctions[]{
			GetIsEnabled,
			GetIntensity,
			GetColor,
			GetAttenuation,
			GetRange,
			GetUmbra,
			GetPenumbra,
			GetType,
			GetEntityID,
		};

		static_assert(_countof(GetFunctions) == LightParameter::count);
#undef CONSTEXPR 
	}

	bool Initialize() {
		return true;
	}

	void Shutdown() {
		assert([] {
			bool has_lights{ false };
			for (const auto& it : light_sets)
				has_lights |= it.second.HasLights();
			return !has_lights;
			}());

		for (u32 i{ 0 }; i < FrameBufferCount; i++)
			light_buffers[i].Release();

	}

	Graphics::Light Create(LightInitInfo info) {
		assert(ID::IsValid(info.entity_id));
		return light_sets[info.light_set_key].Add(info);
	}
	
	void Remove(LightID id, u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		light_sets[light_set_key].Remove(id);
	}

	void SetParameter(LightID id, u64 light_set_key, LightParameter::Parameter param, const void* const data, u32 size) {
		assert(data && size);
		assert(light_sets.count(light_set_key));
		assert(param < LightParameter::count && SetFunctions[param] != DummySet);
		SetFunctions[param](light_sets[light_set_key], id, data, size);
	}
	
	void GetParameter(LightID id, u64 light_set_key, LightParameter::Parameter param, void* const data, u32 size) {
		assert(data && size);
		assert(light_sets.count(light_set_key));
		assert(param < LightParameter::count);
		GetFunctions[param](light_sets[light_set_key], id, data, size);
	}

	void UpdateLightBuffers(const D3D12FrameInfo& d3d12_info) {
		const u64 light_set_key{ d3d12_info.info->light_set_key };
		assert(light_sets.count(light_set_key));
		LightSet& set{ light_sets[light_set_key] };
		if (!set.HasLights()) return;

		set.UpdateTransforms();
		const u32 frame_idx{ d3d12_info.frame_index };
		D3D12LightBuffer& light_buffer{ light_buffers[frame_idx] };
		light_buffer.UpdateLightBuffers(set, light_set_key, frame_idx);
	}

	D3D12_GPU_VIRTUAL_ADDRESS NoncullableLightBuffer(u32 frame_idx) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_idx] };
		return light_buffer.NoncullableLights();
	}

	D3D12_GPU_VIRTUAL_ADDRESS CullableLightBuffer(u32 frame_idx) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_idx] };
		return light_buffer.CullableLights();
	}

	D3D12_GPU_VIRTUAL_ADDRESS CullingInfoBuffer(u32 frame_idx) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_idx] };
		return light_buffer.CullingInfo();
	}

	u32 NoncullableLightCount(u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		return light_sets[light_set_key].NoncullableLightCount();
	}

	u32 CullableLightCount(u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		return light_sets[light_set_key].CullableLightCount();
	}
}