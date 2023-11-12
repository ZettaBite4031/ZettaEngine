#pragma once
#include "../Common/CommonHeaders.h"

namespace Zetta::Platform {
	DEFINE_TYPED_ID(WindowID);
	class Window {
	public:
		constexpr explicit Window(WindowID id) : _id{ id } {}
		constexpr Window() : _id{ ID::Invalid_ID } {}
		constexpr WindowID GetID() const { return _id; }
		const bool IsValid() const { return ID::IsValid(_id); }

		void SetFullScreen(bool) const;
		bool IsFullscreen() const;
		void* Handle() const;
		void SetCaption(const wchar_t*) const;
		const Math::u32v4 Size() const;
		void Resize(u32, u32) const;
		const u32 Width() const;
		const u32 Height() const;
		bool IsClosed() const;

	private:
		WindowID _id{ ID::Invalid_ID };
	};
}
