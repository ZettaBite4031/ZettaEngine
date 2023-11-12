#pragma once
#include "Test.h"
#include "../Platform/PlatformTypes.h"
#include "../Platform/Platform.h"

#ifdef CreateWindow
#undef CreateWindow
#endif

using namespace Zetta;

Platform::Window _windows[4];

LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_DESTROY:
	{
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_windows); i++) {
			if (!_windows[i].IsClosed()) all_closed = false;
		}

		if (all_closed) {
			PostQuitMessage(0);
			return 0;
		}
	}	break;

	case WM_SYSCHAR:
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
			Platform::Window win{ Platform::WindowID{(ID::ID_Type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			win.SetFullScreen(!win.IsFullscreen());
			return 0;
		}

		break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

class WindowTest : public Test {
public:
	bool Initialize() override {

		Platform::WindowInitInfo info[] {
			{&WinProc, nullptr, L"Test Window", 100, 100, 400, 800},
			{&WinProc, nullptr, L"Test Window", 150, 150, 800, 400},
			{&WinProc, nullptr, L"Test Window", 200, 200, 400, 400},
			{&WinProc, nullptr, L"Test Window", 250, 250, 800, 600},
		};

		static_assert(_countof(info) == _countof(_windows));

		for (u32 i{ 0 }; i < _countof(_windows); i++) 
			_windows[i] = Platform::CreateWindow(&info[i]);
		return true;
	}
	
	void Run() override {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	void Shutdown() override {
		for (u32 i{ 0 }; i < _countof(_windows); i++)
			Platform::RemoveWindow(_windows[i].GetID());
	}
};