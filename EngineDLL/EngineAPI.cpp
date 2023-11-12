#include "Common.h"
#include "CommonHeaders.h"
#include "../Engine/Components/Script.h"
#include "../Graphics/Renderer.h"
#include "../Platform/PlatformTypes.h"
#include "../Platform/Platform.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <atlsafe.h>

using namespace Zetta;

namespace {
	HMODULE game_dll{ nullptr };
	using _getScriptCreator = Zetta::Script::Detail::ScriptCreator(*)(size_t);
	_getScriptCreator getScriptCreator{ nullptr };
	using _getScriptNames = LPSAFEARRAY(*)(void);
	_getScriptNames getScriptNames{ nullptr };

	util::vector<Graphics::RenderSurface> surfaces;
}

EDITOR_INTERFACE u32 LoadGameDLL(const char* path) {
	if (game_dll) return FALSE;
	game_dll = LoadLibraryA(path);
	assert(game_dll);

	getScriptCreator = (_getScriptCreator)GetProcAddress(game_dll, "GetScriptCreatorInternal");
	getScriptNames = (_getScriptNames)GetProcAddress(game_dll, "GetScriptNamesInternal");

	return (game_dll && getScriptCreator && getScriptNames) ? TRUE : FALSE;
}

EDITOR_INTERFACE u32 UnloadGameDLL() {
	if (!game_dll) return FALSE;
	assert(game_dll);
	int res{ FreeLibrary(game_dll) };
	assert(res);
	game_dll = nullptr;
	return TRUE;
}

EDITOR_INTERFACE Script::Detail::ScriptCreator GetScriptCreator(const char* name) {
	return (game_dll && getScriptCreator) ? getScriptCreator(Script::Detail::StringHash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY GetScriptNames() {
	return (game_dll && getScriptNames) ? getScriptNames() : nullptr;
}

EDITOR_INTERFACE u32 CreateRenderSurface(HWND host, s32 width, s32 height) {
	assert(host);
	Platform::WindowInitInfo info{ nullptr, host, nullptr, 0, 0, width, height };
	Graphics::RenderSurface surface{ Platform::CreateWindow(&info), {} };
	assert(surface.window.IsValid());
	surfaces.emplace_back(surface);
	return (u32)surfaces.size() - 1;
}

EDITOR_INTERFACE void RemoveRenderSurface(u32 id) {
	assert(id < surfaces.size());
	Platform::RemoveWindow(surfaces[id].window.GetID());
}

EDITOR_INTERFACE HWND GetWindowHandle(u32 id) {
	assert(id < surfaces.size());
	return (HWND)surfaces[id].window.Handle();
}

EDITOR_INTERFACE void ResizeRenderSurface(u32 id) {
	assert(id < surfaces.size());
	surfaces[id].window.Resize(NULL, NULL);
}