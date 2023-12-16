#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "D3D12Core.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"
#include "D3D12Light.h"
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

		pi.Camera.Create = Camera::Create;
		pi.Camera.Remove = Camera::Remove;
		pi.Camera.SetParameter = Camera::SetParameter;
		pi.Camera.GetParameter = Camera::GetParameter;

		pi.Light.Create = Light::Create;
		pi.Light.Remove = Light::Remove;
		pi.Light.SetParameter = Light::SetParameter;
		pi.Light.GetParameter = Light::GetParameter;

		pi.Resources.AddSubmesh = Content::Submesh::Add;
		pi.Resources.RemoveSubmesh = Content::Submesh::Remove;
		pi.Resources.AddMaterial = Content::Material::Add;
		pi.Resources.RemoveMaterial = Content::Material::Remove;
		pi.Resources.AddRenderItem = Content::RenderItem::Add;
		pi.Resources.RemoveRenderItem = Content::RenderItem::Remove;

		pi.platform = GraphicsPlatform::Direct3D12;
	}
}