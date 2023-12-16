#pragma once
#ifdef _WIN64
#include "CommonHeaders.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

namespace Zetta::Input {
	HRESULT ProcessInputMesasge(HWND, UINT, WPARAM, LPARAM);
}

#endif