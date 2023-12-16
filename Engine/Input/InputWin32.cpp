#ifdef _WIN64
#include "InputWin32.h"
#include "Input.h"

namespace Zetta::Input {
	namespace {
		constexpr u32 vk_mapping[256]{
			/* 0x00 */ u32_invalid_id,
			/* 0x01 */ InputCode::MouseLeft,
			/* 0x02 */ InputCode::MouseRight,
			/* 0x03 */ u32_invalid_id,
			/* 0x04 */ InputCode::MouseMiddle,
			/* 0x05 */ u32_invalid_id,
			/* 0x06 */ u32_invalid_id,
			/* 0x07 */ u32_invalid_id,
			/* 0x08 */ InputCode::KeyBackspace,
			/* 0x09 */ InputCode::KeyTab,
			/* 0x0A */ u32_invalid_id,
			/* 0x0B */ u32_invalid_id,
			/* 0x0C */ u32_invalid_id,
			/* 0x0D */ InputCode::KeyReturn,
			/* 0x0E */ u32_invalid_id,
			/* 0x0F */ u32_invalid_id,

            /* 0x10 */ InputCode::KeyShift,
            /* 0x11 */ InputCode::KeyControl,
            /* 0x12 */ InputCode::KeyAlt,
            /* 0x13 */ InputCode::KeyPause,
            /* 0x14 */ InputCode::KeyCapslock,
            /* 0x15 */ u32_invalid_id,
            /* 0x16 */ u32_invalid_id,
            /* 0x17 */ u32_invalid_id,
            /* 0x18 */ u32_invalid_id,
            /* 0x19 */ u32_invalid_id,
            /* 0x1A */ u32_invalid_id,
            /* 0x1B */ InputCode::KeyEscape,
            /* 0x1C */ u32_invalid_id,
            /* 0x1D */ u32_invalid_id,
            /* 0x1E */ u32_invalid_id,
            /* 0x1F */ u32_invalid_id,

            /* 0x20 */ InputCode::KeySpace,
            /* 0x21 */ InputCode::KeyPageUp,
            /* 0x22 */ InputCode::KeyPageDown,
            /* 0x23 */ InputCode::KeyEnd,
            /* 0x24 */ InputCode::KeyHome,
            /* 0x25 */ InputCode::KeyLeft,
            /* 0x26 */ InputCode::KeyUp,
            /* 0x27 */ InputCode::KeyRight,
            /* 0x28 */ InputCode::KeyDown,
            /* 0x29 */ u32_invalid_id,
            /* 0x2A */ u32_invalid_id,
            /* 0x2B */ u32_invalid_id,
            /* 0x2C */ InputCode::KeyPrintScreen,
            /* 0x2D */ InputCode::KeyInsert,
            /* 0x2E */ InputCode::KeyDelete,
            /* 0x2F */ u32_invalid_id,

            /* 0x30 */ InputCode::Key0,
            /* 0x31 */ InputCode::Key1,
            /* 0x32 */ InputCode::Key2,
            /* 0x33 */ InputCode::Key3,
            /* 0x34 */ InputCode::Key4,
            /* 0x35 */ InputCode::Key5,
            /* 0x36 */ InputCode::Key6,
            /* 0x37 */ InputCode::Key7,
            /* 0x38 */ InputCode::Key8,
            /* 0x39 */ InputCode::Key9,
            /* 0x3A */ u32_invalid_id,
            /* 0x3B */ u32_invalid_id,
            /* 0x3C */ u32_invalid_id,
            /* 0x3D */ u32_invalid_id,
            /* 0x3E */ u32_invalid_id,
            /* 0x3F */ u32_invalid_id,

            /* 0x40 */ u32_invalid_id,
            /* 0x41 */ InputCode::KeyA,
            /* 0x42 */ InputCode::KeyB,
            /* 0x43 */ InputCode::KeyC,
            /* 0x44 */ InputCode::KeyD,
            /* 0x45 */ InputCode::KeyE,
            /* 0x46 */ InputCode::KeyF,
            /* 0x47 */ InputCode::KeyG,
            /* 0x48 */ InputCode::KeyH,
            /* 0x49 */ InputCode::KeyI,
            /* 0x4A */ InputCode::KeyJ,
            /* 0x4B */ InputCode::KeyK,
            /* 0x4C */ InputCode::KeyL,
            /* 0x4D */ InputCode::KeyM,
            /* 0x4E */ InputCode::KeyN,
            /* 0x4F */ InputCode::KeyO,

            /* 0x50 */ InputCode::KeyP,
            /* 0x51 */ InputCode::KeyQ,
            /* 0x52 */ InputCode::KeyR,
            /* 0x53 */ InputCode::KeyS,
            /* 0x54 */ InputCode::KeyT,
            /* 0x55 */ InputCode::KeyU,
            /* 0x56 */ InputCode::KeyV,
            /* 0x57 */ InputCode::KeyW,
            /* 0x58 */ InputCode::KeyX,
            /* 0x59 */ InputCode::KeyY,
            /* 0x5A */ InputCode::KeyZ,
            /* 0x5B */ u32_invalid_id,
            /* 0x5C */ u32_invalid_id,
            /* 0x5D */ u32_invalid_id,
            /* 0x5E */ u32_invalid_id,
            /* 0x5F */ u32_invalid_id,

            /* 0x60 */ InputCode::KeyNumpad0,
            /* 0x61 */ InputCode::KeyNumpad1,
            /* 0x62 */ InputCode::KeyNumpad2,
            /* 0x63 */ InputCode::KeyNumpad3,
            /* 0x64 */ InputCode::KeyNumpad4,
            /* 0x65 */ InputCode::KeyNumpad5,
            /* 0x66 */ InputCode::KeyNumpad6,
            /* 0x67 */ InputCode::KeyNumpad7,
            /* 0x68 */ InputCode::KeyNumpad8,
            /* 0x69 */ InputCode::KeyNumpad9,
            /* 0x6A */ InputCode::KeyMultiply,
            /* 0x6B */ InputCode::KeyAdd,
            /* 0x6C */ u32_invalid_id,
            /* 0x6D */ InputCode::KeySubtract,
            /* 0x6E */ InputCode::KeyDecimal,
            /* 0x6F */ InputCode::KeyDivide,

            /* 0x70 */ InputCode::KeyF1,
            /* 0x71 */ InputCode::KeyF2,
            /* 0x72 */ InputCode::KeyF3,
            /* 0x73 */ InputCode::KeyF4,
            /* 0x74 */ InputCode::KeyF5,
            /* 0x75 */ InputCode::KeyF6,
            /* 0x76 */ InputCode::KeyF7,
            /* 0x77 */ InputCode::KeyF8,
            /* 0x78 */ InputCode::KeyF9,
            /* 0x79 */ InputCode::KeyF10,
            /* 0x7A */ InputCode::KeyF11,
            /* 0x7B */ InputCode::KeyF12,
            /* 0x7C */ u32_invalid_id,
            /* 0x7D */ u32_invalid_id,
            /* 0x7E */ u32_invalid_id,
            /* 0x7F */ u32_invalid_id,

            /* 0x80 */ u32_invalid_id,
            /* 0x81 */ u32_invalid_id,
            /* 0x82 */ u32_invalid_id,
            /* 0x83 */ u32_invalid_id,
            /* 0x84 */ u32_invalid_id,
            /* 0x85 */ u32_invalid_id,
            /* 0x86 */ u32_invalid_id,
            /* 0x87 */ u32_invalid_id,
            /* 0x88 */ u32_invalid_id,
            /* 0x89 */ u32_invalid_id,
            /* 0x8A */ u32_invalid_id,
            /* 0x8B */ u32_invalid_id,
            /* 0x8C */ u32_invalid_id,
            /* 0x8D */ u32_invalid_id,
            /* 0x8E */ u32_invalid_id,
            /* 0x8F */ u32_invalid_id,

            /* 0x90 */ InputCode::KeyNumlock,
            /* 0x91 */ InputCode::KeyScrollLock,
            /* 0x92 */ u32_invalid_id,
            /* 0x93 */ u32_invalid_id,
            /* 0x94 */ u32_invalid_id,
            /* 0x95 */ u32_invalid_id,
            /* 0x96 */ u32_invalid_id,
            /* 0x97 */ u32_invalid_id,
            /* 0x98 */ u32_invalid_id,
            /* 0x99 */ u32_invalid_id,
            /* 0x9A */ u32_invalid_id,
            /* 0x9B */ u32_invalid_id,
            /* 0x9C */ u32_invalid_id,
            /* 0x9D */ u32_invalid_id,
            /* 0x9E */ u32_invalid_id,
            /* 0x9F */ u32_invalid_id,

            /* 0xA0 */ u32_invalid_id,
            /* 0xA1 */ u32_invalid_id,
            /* 0xA2 */ u32_invalid_id,
            /* 0xA3 */ u32_invalid_id,
            /* 0xA4 */ u32_invalid_id,
            /* 0xA5 */ u32_invalid_id,
            /* 0xA6 */ u32_invalid_id,
            /* 0xA7 */ u32_invalid_id,
            /* 0xA8 */ u32_invalid_id,
            /* 0xA9 */ u32_invalid_id,
            /* 0xAA */ u32_invalid_id,
            /* 0xAB */ u32_invalid_id,
            /* 0xAC */ u32_invalid_id,
            /* 0xAD */ u32_invalid_id,
            /* 0xAE */ u32_invalid_id,
            /* 0xAF */ u32_invalid_id,

            /* 0xB0 */ u32_invalid_id,
            /* 0xB1 */ u32_invalid_id,
            /* 0xB2 */ u32_invalid_id,
            /* 0xB3 */ u32_invalid_id,
            /* 0xB4 */ u32_invalid_id,
            /* 0xB5 */ u32_invalid_id,
            /* 0xB6 */ u32_invalid_id,
            /* 0xB7 */ u32_invalid_id,
            /* 0xB8 */ u32_invalid_id,
            /* 0xB9 */ u32_invalid_id,
            /* 0xBA */ InputCode::KeyColon,
            /* 0xBB */ InputCode::KeyPlus,
            /* 0xBC */ InputCode::KeyComma,
            /* 0xBD */ InputCode::KeyMinus,
            /* 0xBE */ InputCode::KeyPeriod,
            /* 0xBF */ InputCode::KeyQuestion,

            /* 0xC0 */ InputCode::KeyTilde,
            /* 0xC1 */ u32_invalid_id,
            /* 0xC2 */ u32_invalid_id,
            /* 0xC3 */ u32_invalid_id,
            /* 0xC4 */ u32_invalid_id,
            /* 0xC5 */ u32_invalid_id,
            /* 0xC6 */ u32_invalid_id,
            /* 0xC7 */ u32_invalid_id,
            /* 0xC8 */ u32_invalid_id,
            /* 0xC9 */ u32_invalid_id,
            /* 0xCA */ u32_invalid_id,
            /* 0xCB */ u32_invalid_id,
            /* 0xCC */ u32_invalid_id,
            /* 0xCD */ u32_invalid_id,
            /* 0xCE */ u32_invalid_id,
            /* 0xCF */ u32_invalid_id,

            /* 0xD0 */ u32_invalid_id,
            /* 0xD1 */ u32_invalid_id,
            /* 0xD2 */ u32_invalid_id,
            /* 0xD3 */ u32_invalid_id,
            /* 0xD4 */ u32_invalid_id,
            /* 0xD5 */ u32_invalid_id,
            /* 0xD6 */ u32_invalid_id,
            /* 0xD7 */ u32_invalid_id,
            /* 0xD8 */ u32_invalid_id,
            /* 0xD9 */ u32_invalid_id,
            /* 0xDA */ u32_invalid_id,
            /* 0xDB */ InputCode::KeyBracketOpen,
            /* 0xDC */ InputCode::KeyPipe,
            /* 0xDD */ InputCode::KeyBracketClose,
            /* 0xDE */ InputCode::KeyQuote,
            /* 0xDF */ u32_invalid_id,

            /* 0xE0 */ u32_invalid_id,
            /* 0xE1 */ u32_invalid_id,
            /* 0xE2 */ u32_invalid_id,
            /* 0xE3 */ u32_invalid_id,
            /* 0xE4 */ u32_invalid_id,
            /* 0xE5 */ u32_invalid_id,
            /* 0xE6 */ u32_invalid_id,
            /* 0xE7 */ u32_invalid_id,
            /* 0xE8 */ u32_invalid_id,
            /* 0xE9 */ u32_invalid_id,
            /* 0xEA */ u32_invalid_id,
            /* 0xEB */ u32_invalid_id,
            /* 0xEC */ u32_invalid_id,
            /* 0xED */ u32_invalid_id,
            /* 0xEE */ u32_invalid_id,
            /* 0xEF */ u32_invalid_id,

            /* 0xF0 */ u32_invalid_id,
            /* 0xF1 */ u32_invalid_id,
            /* 0xF2 */ u32_invalid_id,
            /* 0xF3 */ u32_invalid_id,
            /* 0xF4 */ u32_invalid_id,
            /* 0xF5 */ u32_invalid_id,
            /* 0xF6 */ u32_invalid_id,
            /* 0xF7 */ u32_invalid_id,
            /* 0xF8 */ u32_invalid_id,
            /* 0xF9 */ u32_invalid_id,
            /* 0xFA */ u32_invalid_id,
            /* 0xFB */ u32_invalid_id,
            /* 0xFC */ u32_invalid_id,
            /* 0xFD */ u32_invalid_id,
            /* 0xFE */ u32_invalid_id,
            /* 0xFF */ u32_invalid_id,
        };

        struct ModifierFlags {
            enum Flags : u8 {
                LeftShift = 0x10,
                LeftControl = 0x20,
                LeftAlt = 0x40,

                RightShift = 0x01,
                RightControl = 0x02,
                RightAlt = 0x04,
            };
        };

        u8 modifier_key_state{ 0 };

        void SetModifierInput(u8 vk, InputCode::Code code, ModifierFlags::Flags flags) {
            if (GetKeyState(vk) < 0) {
                Set(InputSource::Keyboard, code, { 1.f, 0.f, 0.f });
                modifier_key_state |= flags;
            }
            else if (modifier_key_state & flags) {
                Set(InputSource::Keyboard, code, { 0.f, 0.f, 0.f });
                modifier_key_state &= ~flags;
            }
        }

        void SetModifierInputs(InputCode::Code code) {
            if (code == InputCode::KeyShift) {
                SetModifierInput(VK_LSHIFT, InputCode::KeyLeftShift, ModifierFlags::LeftShift);
                SetModifierInput(VK_RSHIFT, InputCode::KeyRightShift, ModifierFlags::RightShift);
            }
            else if (code == InputCode::KeyControl) {
                SetModifierInput(VK_LCONTROL, InputCode::KeyLeftControl, ModifierFlags::LeftControl);
                SetModifierInput(VK_RCONTROL, InputCode::KeyRightControl, ModifierFlags::RightControl);
            }
            else if (code == InputCode::KeyAlt) {
                SetModifierInput(VK_LMENU, InputCode::KeyLeftAlt, ModifierFlags::LeftAlt);
                SetModifierInput(VK_RMENU, InputCode::KeyRightAlt, ModifierFlags::RightAlt);
            }
        }

        constexpr Math::v2 GetMousePosition(LPARAM lparam) {
            return { (f32)((s16)(lparam & 0x0000FFFF)), (f32)((s16)(lparam >> 16)) };
        }
        
    }

    HRESULT ProcessInputMesasge(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            assert(wparam <= 0xFF);
            const InputCode::Code code{ vk_mapping[wparam & 0xFF] };
            if (code != u32_invalid_id) {
                Set(InputSource::Keyboard, code, { 1.f, 0.f, 0.f });
                SetModifierInputs(code);
            }
        }
        break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            assert(wparam <= 0xFF);
            const InputCode::Code code{ vk_mapping[wparam & 0xFF] };
            if (code != u32_invalid_id) {
                Set(InputSource::Keyboard, code, { 0.f, 0.f, 0.f });
                SetModifierInputs(code);
            }
        }
        break;
        case WM_MOUSEMOVE:
        {
            const Math::v2 pos{ GetMousePosition(lparam) };
            Set(InputSource::Mouse, InputCode::MousePositionX, { pos.x, 0.f, 0.f });
            Set(InputSource::Mouse, InputCode::MousePositionY, { pos.y, 0.f, 0.f });
            Set(InputSource::Mouse, InputCode::MousePosition, { pos.x, pos.y, 0.f });
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            SetCapture(hwnd);
            const InputCode::Code code{ msg == WM_LBUTTONDOWN ? InputCode::MouseLeft : msg == WM_RBUTTONDOWN ? InputCode::MouseRight : InputCode::MouseMiddle };
            const Math::v2 pos{ GetMousePosition(lparam) };
            Set(InputSource::Mouse, code, { pos.x, pos.y, 1.f });
        }
        break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            ReleaseCapture();
            const InputCode::Code code{ msg == WM_LBUTTONUP ? InputCode::MouseLeft : msg == WM_RBUTTONUP ? InputCode::MouseRight : InputCode::MouseMiddle };
            const Math::v2 pos{ GetMousePosition(lparam) };
            Set(InputSource::Mouse, code, { pos.x, pos.y, 0.f });
        }
        break;
        case WM_MOUSEWHEEL:
        {
            Set(InputSource::Mouse, InputCode::MouseWheel, { (f32)(GET_WHEEL_DELTA_WPARAM(wparam)), 0.f, 0.f });
        }
        break;
        }
        return S_OK;
    }
}

#endif