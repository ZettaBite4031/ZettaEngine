#pragma once
#include "CommonHeaders.h"
#include "../Platform/Window.h"

namespace Zetta::Graphics {
	class Surface {
		
	};

	struct RenderSurface {
		Platform::Window window{};
		Surface surface{};
	};
}