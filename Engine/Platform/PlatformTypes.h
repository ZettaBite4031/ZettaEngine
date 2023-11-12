#pragma once
#include "../Common/CommonHeaders.h"

#ifdef _WIN64
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Zetta::Platform {
	using WindowProc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
	using WindowHandle = HWND;

	struct WindowInitInfo {
		WindowProc		callback{ nullptr };
		WindowHandle	parent{ nullptr };
		const wchar_t*		caption{ nullptr };
		s32				left{ 0 };
		s32				top{ 0 };
		s32				width{ 1920 };
		s32				height{ 1080 };
	};
}


#endif