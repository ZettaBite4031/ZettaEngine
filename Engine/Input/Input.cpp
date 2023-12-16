#include "Input.h"

namespace Zetta::Input {
	namespace {
		struct InputBinding {
			util::vector<InputSource> sources;
			InputValue value{};
			bool is_dirty{ true };
		};

		std::unordered_map<u64, InputValue> input_values;
		std::unordered_map<u64, InputBinding> input_bindings;
		std::unordered_map<u64, u64> source_binding_map;
		util::vector<Detail::InputSystemBase*> input_callbacks;

		constexpr u64 GetKey(InputSource::Type type, u32 code) {
			return ((u64)type << 32) | (u64)code;
		}

	} // anonymous namespace

	void Bind(InputSource source) {
		assert(source.source_type < InputSource::count);
		const u64 key{ GetKey(source.source_type, source.code) };
		Unbind(source.source_type, (InputCode::Code)source.code);

		input_bindings[source.binding].sources.emplace_back(source);
		source_binding_map[key] = source.binding;
	}

	void Unbind(InputSource::Type type, InputCode::Code code) {
		assert(type < InputSource::count);
		const u64 key{ GetKey(type, code) };
		if (!source_binding_map.count(key)) return;

		const u64 binding_key{ source_binding_map[key] };
		assert(input_bindings.count(binding_key));
		InputBinding& binding{ input_bindings[binding_key] };
		util::vector<InputSource>& sources{ binding.sources };
		for (u32 i{ 0 }; i < sources.size(); i++) {
			if (sources[i].source_type == type && sources[i].code == code) {
				assert(sources[i].binding = source_binding_map[key]);
				util::EraseUnordered(sources, i);
				source_binding_map.erase(key);
				break;
			}
		}

		if (!sources.size()) {
			assert(!source_binding_map.count(key));
			input_bindings.erase(binding_key);
		}

	}

	void Unbind(u64 binding) {
		if (!input_bindings.count(binding)) return;

		util::vector<InputSource>& sources{ input_bindings[binding].sources };
		for (const auto& source : sources) {
			assert(source.binding == binding);
			const u64 key{ GetKey(source.source_type, source.code) };
			assert(source_binding_map.count(key) && source_binding_map[key] == binding);
			source_binding_map.erase(key);
		}
		input_bindings.erase(binding);
	}

	void Set(InputSource::Type type, InputCode::Code code, Math::v3 value) {
		assert(type < InputSource::count);
		const u64 key{ GetKey(type, code) };
		InputValue& input{ input_values[key] };
		input.previous = input.current;
		input.current = value;

		if (source_binding_map.count(key)) {
			const u64 binding_key{ source_binding_map[key] };
			assert(input_bindings.count(binding_key));
			InputBinding& binding{ input_bindings[binding_key] };
			binding.is_dirty = true;

			InputValue binding_value;
			Get(binding_key, binding_value);

			for (const auto& c : input_callbacks) c->OnEvent(binding_key, binding_value);
		}

		// TODO: The callbacks could cause race conditions in scripts when not run on the same thread as the game scripts.
		for (const auto& c : input_callbacks) 
			c->OnEvent(type, code, input);
	}

	void Get(InputSource::Type type, InputCode::Code code, InputValue& value) {
		assert(type < InputSource::count);
		const u64 key{ GetKey(type, code) };
		value = input_values[key];
	}

	void Get(u64 binding, InputValue& value) {
		if (!input_bindings.count(binding)) return;

		InputBinding& input_binding{ input_bindings[binding] };
		if (!input_binding.is_dirty) {
			value = input_binding.value;
			return;
		}

		util::vector<InputSource>& sources{ input_binding.sources};
		InputValue sub_value{};
		InputValue result{};

		for (const auto& source : sources) {
			assert(source.binding == binding);
			Get(source.source_type, (InputCode::Code)source.code, sub_value);
			assert(source.axis <= Axis::Z);
			if (source.source_type == InputSource::Mouse) {
				const f32 current{ (&sub_value.current.x)[source.source_axis] };
				const f32 previous{ (&sub_value.previous.x)[source.source_axis] };
				(&result.current.x)[source.axis] += (current - previous) * source.multiplier;
			}
			else {
				(&result.previous.x)[source.axis] += (&sub_value.previous.x)[source.source_axis] * source.multiplier;
				(&result.current.x)[source.axis] += (&sub_value.current.x)[source.source_axis] * source.multiplier;
			}
		}

		input_binding.value = result;
		input_binding.is_dirty = false;
		value = result;
	}

	Detail::InputSystemBase::InputSystemBase() {
		input_callbacks.emplace_back(this);
	}

	Detail::InputSystemBase::~InputSystemBase() {
		for (u32 i{ 0 }; i < input_callbacks.size(); i++) {
			if (input_callbacks[i] == this) {
				util::EraseUnordered(input_callbacks, i);
				break;
			}
		}
	}
}