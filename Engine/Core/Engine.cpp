#if !defined(SHIPPING)
#include "../Content/ContentLoader.h"
#include "../Components/Script.h"	
#include "../Platform/PlatformTypes.h"
#include "../Platform/Platform.h"
#include "../Graphics/Renderer.h"
#include <thread>

using namespace Zetta;

namespace {
	Graphics::RenderSurface game_window{};

	LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		switch (msg) {
		case WM_DESTROY:
		{
			if (game_window.window.IsClosed()) {
				PostQuitMessage(0);
				return 0;
			}
		}	break;

		case WM_SYSCHAR:
			if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
				game_window.window.SetFullScreen(!game_window.window.IsFullscreen());
				return 0;
			}

			break;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

bool engine_initialize() {
	if (!Content::LoadGame()) return false;

	Platform::WindowInitInfo info{
		&WinProc, nullptr, L"Zetta Game" // TODO: Get game name from binary 
	};

	game_window.window = Platform::CreateWindow(&info);
	if (!game_window.window.IsValid()) return false;

	return true;
}

void engine_update() {
	Zetta::Script::Update(10.0f);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown() {
	Platform::RemoveWindow(game_window.window.GetID());
	Zetta::Content::UnloadGame();
}
#endif