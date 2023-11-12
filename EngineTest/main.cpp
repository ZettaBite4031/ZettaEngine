#pragma comment(lib, "Engine.lib")

//#define TEST_ENTITY_COMPONENTS 1
#define TEST_WINDOW 1

#if TEST_ENTITY_COMPONENTS
#include "EntityComponentTest.h"

int main() {
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	EntityComponentTest test{};

	if (test.Initialize()) test.Run();
	test.Shutdown();

}


#elif TEST_WINDOW
#include "WindowTest.h"
#ifdef _WIN64
#include <Windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WindowTest test{};

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
#else
int main() {
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WindowTest test{};

	if (test.Initialize()) test.Run();
	test.Shutdown();

	return 0;
}
#endif
#else
#error At least one test must be enabled
#endif