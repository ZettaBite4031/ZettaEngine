#pragma once
#include "CommonHeaders.h"
#include "Graphics/Renderer.h"
#include "Platform/Window.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace Zetta::Graphics::D3D12 {
	constexpr u32 FrameBufferCount{ 3 };
	using ID3D12Device = ID3D12Device8;
	using ID3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
}

#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)							\
if (FAILED(x)) {							\
	char line_number[32];					\
	sprintf_s(line_number, "%u", __LINE__);	\
	OutputDebugStringA("Error in: ");		\
	OutputDebugStringA(__FILE__);			\
	OutputDebugStringA("\nLine: ");			\
	OutputDebugStringA(line_number);		\
	OutputDebugStringA("\n");				\
	OutputDebugStringA(#x);					\
	OutputDebugStringA("\n");				\
	__debugbreak();							\
}
#endif
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Create: "); OutputDebugString(name); OutputDebugString(L"\n");
#define NAME_D3D12_OBJECT_INDEXED(obj, idx, name)			\
{															\
	wchar_t full_name[128];									\
	if (swprintf_s(full_name, L"%s[%u]", name, idx) > 0) {	\
		obj->SetName(full_name);							\
		OutputDebugString(L"::D3D12 Object Created: ");		\
		OutputDebugString(full_name);						\
		OutputDebugString(L"\n");							\
	}														\
}
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#define NAME_D3D12_OBJECT(obj, name) ((void)0)
#define NAME_D3D12_OBJECT_INDEXED(obj, idx, name) ((void)0)
#endif

#include "D3D12Helpers.h"
#include "D3D12Resources.h"