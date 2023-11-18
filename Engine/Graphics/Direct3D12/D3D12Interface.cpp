#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "D3D12Core.h"
#include "Graphics/GraphicsPlatformInterface.h"


namespace Zetta::Graphics::D3D12 {
	void  GetPlatformInterface(PlatformInterface& pi) {
		pi.Initialize = Core::Initialize;
		pi.Shutdown = Core::Shutdown;

		pi.Surface.Create = Core::CreateSurface;
		pi.Surface.Remove = Core::RemoveSurface;
		pi.Surface.Resize = Core::ResizeSurface;
		pi.Surface.Width = Core::SurfaceWidth;
		pi.Surface.Height = Core::SurfaceHeight;
		pi.Surface.Render = Core::RenderSurface;

		pi.platform = GraphicsPlatform::Direct3D12;
	}
}