#pragma once
#include "Common/CommonHeaders.h"
#include "Window.h"

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Zetta::Platform{

	struct WindowInitInfo;

	Window CreateWindow(const WindowInitInfo* const info = nullptr);
	void RemoveWindow(WindowID);
}