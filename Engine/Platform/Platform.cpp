#include "Platform.h"
#include "PlatformTypes.h"

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Zetta::Platform {
#ifdef _WIN64

	namespace {
		struct WindowInfo {
			HWND hwnd{ nullptr };
			RECT client_area{ 0, 0, 960, 540 };
			RECT fullscreen_area{};
			POINT top_left{ 0,0 };
			DWORD style{ WS_VISIBLE };
			bool is_fullscreen{ false };
			bool is_closed{ false };
		};

		util::vector<WindowInfo> windows;

		util::vector<u32> available_slots;
		u32 AddToWindows(WindowInfo info) {
			u32 id{ u32_invalid_id };
			if (available_slots.empty()) {
				id = (u32)windows.size();
				windows.emplace_back(info);
			}
			else {
				id = available_slots.back();
				available_slots.pop_back();
				assert(id != u32_invalid_id);
				windows[id] = info;
			}
			return id;
		}

		void RemoveFromWindows(u32 id) {
			assert(id < windows.size());
			available_slots.emplace_back(id);
		}

		WindowInfo& GetFromID(WindowID id) {
			assert(id < windows.size());
			assert(windows[id].hwnd);
			return windows[id];
		}

		WindowInfo& GetFromHandle(WindowHandle hnd) {
			const WindowID id{ (ID::ID_Type)GetWindowLongPtr(hnd, GWLP_USERDATA) };
			return GetFromID(id);
		}

		LRESULT CALLBACK InternalWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			WindowInfo* info{ nullptr };

			switch (msg) {
			case WM_DESTROY:
				GetFromHandle(hwnd).is_closed = true;
				break;
			case WM_EXITSIZEMOVE:
				info = &GetFromHandle(hwnd);
				break;
			case WM_SIZE:
				if (wparam == SIZE_MAXIMIZED) 
					info = &GetFromHandle(hwnd);
				break;
			case WM_SYSCOMMAND:
				if (wparam == SC_RESTORE)
					info = &GetFromHandle(hwnd);
				break;
			default:
				break;
			}

			if (info) {
				assert(info->hwnd);
				GetClientRect(info->hwnd, info->is_fullscreen ? &info->fullscreen_area : &info->client_area);
			}

			LONG_PTR long_ptr{ GetWindowLongPtr(hwnd, 0) };
			return long_ptr ? 
				((WindowProc)long_ptr)(hwnd, msg, wparam, lparam) 
				: DefWindowProc(hwnd, msg, wparam, lparam);
		}

		void ResizeWindow(const WindowInfo& info, const RECT& area) {
			RECT window_rect{ area };
			AdjustWindowRect(&window_rect, info.style, FALSE);

			const s32 width{ window_rect.right - window_rect.left };
			const s32 height{ window_rect.bottom - window_rect.top };

			MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, true);
		}

		void ResizeWindow(WindowID id, u32 width, u32 height) {
			WindowInfo& info{ GetFromID(id) };
			if (info.style & WS_CHILD) GetClientRect(info.hwnd, &info.client_area);
			else {
				RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
				area.bottom = area.top + height;
				area.right = area.left + width;
				ResizeWindow(info, area);
			}
		}

		void SetWindowFullscreen(WindowID id, bool fs) {
			WindowInfo& info{ GetFromID(id) };
			if (info.is_fullscreen != fs) {
				info.is_fullscreen = fs;

				if (fs) {
					GetClientRect(info.hwnd, &info.client_area);
					RECT rect;
					GetWindowRect(info.hwnd, &rect);
					info.top_left.x = rect.left;
					info.top_left.y = rect.top;
					SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
					ShowWindow(info.hwnd, SW_MAXIMIZE);
				}
				else {
					SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
					ResizeWindow(info, info.client_area);
					ShowWindow(info.hwnd, SW_SHOWNORMAL);
				}
			}
		}

		bool IsWindowFullscreen(WindowID id) {
			return GetFromID(id).is_fullscreen;
		}

		void* GetWindowHandle(WindowID id) {
			return GetFromID(id).hwnd;
		}

		void SetWindowCaption(WindowID id, const wchar_t* c) {
			WindowInfo& info{ GetFromID(id) };
			SetWindowText(info.hwnd, c);
		}

		Math::u32v4 GetWindowSize(WindowID id) {
			WindowInfo& info{ GetFromID(id) };
			RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
			return { (u32)area.left, (u32)area.top, (u32)area.right, (u32)area.bottom };
		}

		bool IsWindowClosed(WindowID id) {
			return GetFromID(id).is_closed;
		}
	}

	Window CreateWindow(const WindowInitInfo* const info) {
		WindowProc callback{ info ? info->callback : nullptr };
		WindowHandle parent{ info ? info->parent : nullptr };

		// Setup window class
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = InternalWindowProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = callback ? sizeof(callback) : 0;
		wc.hInstance = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"ZettaWindow";
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		// Register window class
		RegisterClassEx(&wc);

		// Create an instance of the window class
		WindowInfo winfo{};
		winfo.client_area.right = (info && info->width) ? winfo.client_area.left + info->width : winfo.client_area.right;
		winfo.client_area.bottom = (info && info->height) ? winfo.client_area.top + info->height : winfo.client_area.bottom;
		RECT rect{ winfo.client_area };
		winfo.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;
		AdjustWindowRect(&rect, winfo.style, FALSE);

		const wchar_t* caption{ (info && info->caption) ? info->caption : L"Zetta Game" };
		const s32 left{ (info) ? info->left : winfo.top_left.x };
		const s32 top{ (info) ? info->top : winfo.top_left.y};
		const s32 right{ rect.right - rect.left };
		const s32 bottom{ rect.bottom - rect.top };

		winfo.hwnd = CreateWindowEx(
			/* DWORD	 */		0,					// extended style
			/* LPCSTR	 */		wc.lpszClassName,	// window class name
			/* LPCSTR	 */		caption,			// instance title
			/* DWORD	 */		winfo.style,		// window style
			/* int		 */		left,				// initial window x pos
			/* int		 */		top,				// initial window y pos
			/* int		 */		right,				// initial window width
			/* int		 */		bottom,				// initial window height
			/* HWND		 */		parent,				// parent window handle
			/* HMENU	 */		NULL,				// menu handle
			/* HINSTANCE */		NULL,				// application instance
			/* LPVOID	 */		NULL				// extra creation params
		);

		if (winfo.hwnd) {
			DEBUG_OP(SetLastError(0));

			WindowID id{ AddToWindows(winfo) };
			SetWindowLongPtr(winfo.hwnd, GWLP_USERDATA, (LONG_PTR)id);

			if(callback) SetWindowLongPtr(winfo.hwnd, 0, (LONG_PTR)callback);
			assert(GetLastError() == 0);

			ShowWindow(winfo.hwnd, SW_SHOWNORMAL);
			UpdateWindow(winfo.hwnd);
			return Window{ id };
		}
		return {};
	}

	void RemoveWindow(WindowID id) {
		WindowInfo& info{ GetFromID(id) };
		DestroyWindow(info.hwnd);
		RemoveFromWindows(id);
	}

#elif
#else
#error At least one platform must be implmented
#endif // _WIN64

	void Window::SetFullScreen(bool fs) const{
		assert(IsValid());
		SetWindowFullscreen(_id, fs);
	}

	bool Window::IsFullscreen() const{
		assert(IsValid());
		return IsWindowFullscreen(_id);
	}

	void* Window::Handle() const{
		assert(IsValid());
		return GetWindowHandle(_id);
	}

	void Window::SetCaption(const wchar_t* c) const{
		assert(IsValid());
		SetWindowCaption(_id, c);
	}

	const Math::u32v4 Window::Size() const{
		assert(IsValid());
		return GetWindowSize(_id);
	}

	void Window::Resize(u32 w, u32 h) const{
		assert(IsValid());
		ResizeWindow(_id, w, h);
	}

	const u32 Window::Width() const{
		Math::u32v4 s{ Size() };
		return s.z - s.x;
	}

	const u32 Window::Height() const{
		Math::u32v4 s{ Size() };
		return s.w - s.y;
	}

	bool Window::IsClosed() const {
		assert(IsValid());
		return IsWindowClosed(_id);
	}
}