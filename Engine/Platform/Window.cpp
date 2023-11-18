#if INCLUDE_WINDOW_CPP
#include "Window.h"

namespace Zetta {
	namespace Platform {

		void Window::SetFullScreen(bool fs) const {
			assert(IsValid());
			SetWindowFullscreen(_id, fs);
		}

		bool Window::IsFullscreen() const {
			assert(IsValid());
			return IsWindowFullscreen(_id);
		}

		void* Window::Handle() const {
			assert(IsValid());
			return GetWindowHandle(_id);
		}

		void Window::SetCaption(const wchar_t* c) const {
			assert(IsValid());
			SetWindowCaption(_id, c);
		}

		const Math::u32v4 Window::Size() const {
			assert(IsValid());
			return GetWindowSize(_id);
		}

		void Window::Resize(u32 w, u32 h) const {
			assert(IsValid());
			ResizeWindow(_id, w, h);
		}

		const u32 Window::Width() const {
			Math::u32v4 s{ Size() };
			return s.z - s.x;
		}

		const u32 Window::Height() const {
			Math::u32v4 s{ Size() };
			return s.w - s.y;
		}

		bool Window::IsClosed() const {
			assert(IsValid());
			return IsWindowClosed(_id);
		}
	}
}
#endif