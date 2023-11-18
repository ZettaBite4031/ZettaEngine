#include "Test.h"

#pragma comment(lib, "Engine.lib")

#if TEST_ENTITY_COMPONENTS
#include "EntityComponentTest.h"
#elif TEST_WINDOW
#include "WindowTest.h"
#elif TEST_RENDERER
#include "RendererTest.h"
#else
#error At least one test must be enabled
#endif


#ifdef _WIN64
#include <Windows.h>
#include <filesystem>

std::filesystem::path SetCurrentDirToExePath() {
	// set the working directory to exe path
	wchar_t path[MAX_PATH]{};
	const uint32_t length{ GetModuleFileName(0, &path[0], MAX_PATH) };
	if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return{};
	std::filesystem::path p{ path };
	std::filesystem::current_path(p.parent_path());
	return std::filesystem::current_path();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	SetCurrentDirToExePath();

	EngineTest test{};

	if (test.Initialize()) {
		MSG msg{};
		bool is_running{ true };
		while (is_running) {
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				is_running &= (msg.message != WM_QUIT);
			}
			test.Run();
		}
	}
	test.Shutdown();
	return 0;
}
#endif
