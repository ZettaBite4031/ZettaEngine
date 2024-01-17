#pragma once

namespace Zetta::Graphics::Vulkan {
	constexpr const char* validation_layers[]{
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef _DEBUG
	constexpr bool enable_validation_layers{ true };
#else 
	constexpr bool enable_validation_layers{ false };
#endif

}