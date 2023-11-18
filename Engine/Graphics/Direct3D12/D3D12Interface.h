#pragma once

namespace Zetta::Graphics {
	struct PlatformInterface;

	namespace D3D12 {
		void GetPlatformInterface(PlatformInterface& pi);
	}
}