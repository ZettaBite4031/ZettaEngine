#include "../Platform/PlatformTypes.h"
#include "../Platform/Platform.h"
#include "../Graphics/Renderer.h"
#include "RendererTest.h"
#include "ShaderCompilation.h"

#if TEST_RENDERER
using namespace Zetta;

Graphics::RenderSurface _surfaces[4];
TimeIt timer{};

bool resized{ false };
bool is_restarting{ false };
void DestroyRenderSurface(Graphics::RenderSurface& surface);

bool TestInitialize();
void TestShutdown();

LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	bool toggle_fullscreen{ false };

	switch (msg) {
	case WM_DESTROY:
	{
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (_surfaces[i].window.IsValid()) {
				if (_surfaces[i].window.IsClosed()) DestroyRenderSurface(_surfaces[i]);
				else all_closed = false;
			}
		}

		if (all_closed && !is_restarting) {
			PostQuitMessage(0);
			return 0;
		}

	}	break;

	case WM_SIZE:
		resized = (wparam != SIZE_MINIMIZED);
		break;

	case WM_SYSCHAR:
		toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
		break;

	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wparam == VK_F11) {
			is_restarting = true;
			TestShutdown();
			TestInitialize();
		}
		break;
	}

	if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen) {
		Platform::Window win{ Platform::WindowID{ (ID::ID_Type)GetWindowLongPtr(hwnd, GWLP_USERDATA) } };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (win.GetID() == _surfaces[i].window.GetID()) {
				if (toggle_fullscreen) {
					win.SetFullScreen(!win.IsFullscreen());
					return 0;
				}
				else {
					_surfaces[i].surface.Resize(win.Width(), win.Height());
					resized = false;
				}
				break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void CreateRenderSurface(Graphics::RenderSurface& surface, Platform::WindowInitInfo& info) {
	surface.window = Platform::CreateWindow(&info);
	surface.surface = Graphics::CreateSurface(surface.window);
}

void DestroyRenderSurface(Graphics::RenderSurface& surface) {
	Graphics::RenderSurface temp{ surface };
	surface = {};
	if(temp.surface.IsValid()) Graphics::RemoveSurface(temp.surface.GetID());
	if(temp.window.IsValid()) Platform::RemoveWindow(temp.window.GetID());
}

bool TestInitialize() {
	while (!CompileShaders()) {
		if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
			return false;
	}

	if (!Graphics::Initialize(Graphics::GraphicsPlatform::Direct3D12)) return false;
	Platform::WindowInitInfo info[]{
		{&WinProc, nullptr, L"Test Window", 100 + 400, 100, 400, 800},
		{&WinProc, nullptr, L"Test Window", 150 + 400, 150, 800, 400},
		{&WinProc, nullptr, L"Test Window", 200 + 400, 200, 400, 400},
		{&WinProc, nullptr, L"Test Window", 250 + 400, 250, 800, 600},
	};

	static_assert(_countof(info) == _countof(_surfaces));

	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		CreateRenderSurface(_surfaces[i], info[i]);

	is_restarting = false;
	return true;
}

void TestShutdown() {
	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		DestroyRenderSurface(_surfaces[i]);

	Graphics::Shutdown();
}

bool EngineTest::Initialize()  {
	return TestInitialize();
}

void EngineTest::Run()  {
	timer.Begin();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		if(_surfaces[i].surface.IsValid())
			_surfaces[i].surface.Render();
	timer.End();
}

void EngineTest::Shutdown()  {
	TestShutdown();
}

#endif