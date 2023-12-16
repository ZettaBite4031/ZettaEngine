#pragma once
#include "CommonHeaders.h"

namespace Zetta::Input {
	struct Axis {
		enum Type : u32 {
			X = 0, 
			Y = 1,
			Z = 2,
		};
	};

	struct ModifierKey {
		enum Key : u32 {
			None = 0x00,
			LeftShift = 0x01,
			RightShift = 0x02,
			Shift = LeftShift | RightShift,
			LeftControl = 0x04,
			RightControl = 0x08,
			Control = LeftControl | RightControl,
			LeftAlt = 0x10,
			RightAlt = 0x20,
			Alt = LeftAlt | RightAlt
		};
	};

	struct InputValue {
		Math::v3 previous;
		Math::v3 current;
	};

	struct InputCode {
		enum Code : u32 {
			MousePosition,
			MousePositionX,
			MousePositionY,
			MouseLeft,
			MouseRight,
			MouseMiddle,
			MouseWheel,

			KeyBackspace,
			KeyTab,
			KeyReturn,
			KeyShift,
			KeyLeftShift,
			KeyRightShift,
			KeyControl,
			KeyLeftControl,
			KeyRightControl,
			KeyAlt,
			KeyLeftAlt,
			KeyRightAlt,
			KeyPause,
			KeyCapslock,
			KeyEscape,
			KeySpace,
			KeyPageUp,
			KeyPageDown,
			KeyHome,
			KeyEnd,
			KeyLeft,
			KeyUp,
			KeyRight,
			KeyDown,
			KeyPrintScreen,
			KeyInsert,
			KeyDelete,

			Key0,
			Key1,
			Key2,
			Key3,
			Key4,
			Key5,
			Key6,
			Key7,
			Key8,
			Key9,

			KeyA,
			KeyB,
			KeyC,
			KeyD,
			KeyE,
			KeyF,
			KeyG,
			KeyH,
			KeyI,
			KeyJ,
			KeyK,
			KeyL,
			KeyM,
			KeyN,
			KeyO,
			KeyP,
			KeyQ,
			KeyR,
			KeyS,
			KeyT,
			KeyU,
			KeyV,
			KeyW,
			KeyX,
			KeyY,
			KeyZ,

			KeyNumpad0,
			KeyNumpad1,
			KeyNumpad2,
			KeyNumpad3,
			KeyNumpad4,
			KeyNumpad5,
			KeyNumpad6,
			KeyNumpad7,
			KeyNumpad8,
			KeyNumpad9,

			KeyMultiply,
			KeyAdd,
			KeySubtract,
			KeyDecimal,
			KeyDivide,

			KeyF1,
			KeyF2,
			KeyF3,
			KeyF4,
			KeyF5,
			KeyF6,
			KeyF7,
			KeyF8,
			KeyF9,
			KeyF10,
			KeyF11,
			KeyF12,

			KeyNumlock,
			KeyScrollLock,
			KeyColon,
			KeyPlus,
			KeyComma,
			KeyMinus,
			KeyPeriod,
			KeyQuestion,
			KeyBracketOpen,
			KeyPipe,
			KeyBracketClose,
			KeyQuote,
			KeyTilde,
		};
	};

	struct InputSource {
		enum Type : u32 {
			Keyboard,
			Mouse,
			Controller,
			Raw,

			count
		};

		u64 binding{ 0 };
		Type source_type{};
		u32 code{ 0 };
		float multiplier{ 0 };
		bool is_discrete{ true };
		Axis::Type source_axis{};
		Axis::Type axis{};
		ModifierKey::Key modifier{};
	};

	void Get(InputSource::Type type, InputCode::Code code, InputValue& value);
	void Get(u64, InputValue&);

	namespace Detail {
		class InputSystemBase {
		public:
			virtual void OnEvent(InputSource::Type, InputCode::Code, const InputValue&) = 0;
			virtual void OnEvent(u64 , const InputValue&) = 0;
		protected:
			InputSystemBase();
			~InputSystemBase();
		};
	}

	template<typename T>
	class InputSystem final : public Detail::InputSystemBase {
	public:
		using InputCallback_t = void(T::*)(InputSource::Type, InputCode::Code, const InputValue&);
		using BindingCallback_t = void(T::*)(u64, const InputValue&);

		void AddHandler(InputSource::Type type, T* instance, InputCallback_t callback) {
			assert(instance && callback && type < InputSource::count);
			auto& collection = _input_callbacks[type];
			for (const auto& func : collection) {
				// If handler already exists, don't add it again
				if (func.instance == instance && func.callback == callback) return;
			}

			collection.emplace_back(InputCallback{ instance, callback });
		}

		void AddHandler(u64 binding, T* instance, BindingCallback_t callback) {
			assert(instance && callback);
			for (const auto& func : _binding_callbacks) {
				// if Handler already exists, don't add it again
				if (func.binding == binding && func.instance == instance && func.callback == callback) return;
			}

			_binding_callbacks.emplace_back(BindingCallback{ binding, instance, callback });
		}

		virtual void OnEvent(InputSource::Type type, InputCode::Code code, const InputValue& value) override {
			assert(type < InputSource::count);
			for (const auto& item : _input_callbacks[type]) {
				(item.instance->*item.callback)(type, code, value);
			}
		}

		virtual void OnEvent(u64 binding, const InputValue& value) override {
			for (const auto& item : _binding_callbacks) {
				if (item.binding == binding) {
					(item.instance->*item.callback)(binding, value);
				}
			}
		}

	private:
		struct InputCallback {
			T*				instance;
			InputCallback_t callback;
		};

		struct BindingCallback {
			u64				binding;
			T*				instance;
			BindingCallback_t callback;
		};

		util::vector<InputCallback> _input_callbacks[InputSource::count];
		util::vector<BindingCallback> _binding_callbacks;
	};
}