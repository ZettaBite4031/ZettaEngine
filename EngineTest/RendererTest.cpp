#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Graphics/Direct3D12/D3D12Core.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "RendererTest.h"
#include "ShaderCompilation.h"

#include <fstream>
#include <filesystem>

#if TEST_RENDERER
using namespace Zetta;

#pragma region MultiThreaded Test Worker Spawn
// MultiThreading Test Worker Spawn /////////////////////////////////////////////////////////////////////////////
#define ENABLE_TEST_WORKERS 0

constexpr u32 num_threads{ 8 };
bool shutdown{ false };
std::thread workers[num_threads];

util::vector<u8> buffer(1024 * 1024, 0);
// Test worker for upload context
void BufferTestWorker() {
	while (!shutdown) {
		auto* resource = Graphics::D3D12::D3DX::CreateBuffer(buffer.data(), (u32)buffer.size());



		Graphics::D3D12::Core::DeferredRelease(resource);
	}
}

template<class FnPtr, class... Args>
void InitTestWorkers(FnPtr&& fnPtr, Args&&... args) {
#if ENABLE_TEST_WORKERS
	shutdown = false;
	for (auto& w : workers) 
		w = std::thread(std::forward<FnPtr>(fnPtr), std::forward<Args>(args)...);
#endif
}

void JoinTestWorkers() {
#if ENABLE_TEST_WORKERS
	shutdown = true;
	for (auto& w : workers) w.join();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma endregion

struct {
	GameEntity::Entity entity{};
	Graphics::Camera camera{};
} camera;

ID::ID_Type model_id{ ID::Invalid_ID };
ID::ID_Type item_id{ ID::Invalid_ID };

Graphics::RenderSurface _surfaces[4];
TimeIt timer{};

bool resized{ false };
bool is_restarting{ false };
void DestroyRenderSurface(Graphics::RenderSurface& surface);

bool TestInitialize();
void TestShutdown();

ID::ID_Type CreateRenderItem(ID::ID_Type entity_id);
void DestroyRenderItem(ID::ID_Type render_item_id);

LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	bool toggle_fullscreen{ false };

	switch (msg) {
	case WM_DESTROY:
	{
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (_surfaces[i].window.IsValid()) {
				if (_surfaces[i].window.IsClosed()) DestroyRenderSurface(_surfaces[i]);
				else all_closed = false;
			}
		}

		if (all_closed && !is_restarting) {
			PostQuitMessage(0);
			return 0;
		}

	}	break;

	case WM_SIZE:
		resized = (wparam != SIZE_MINIMIZED);
		break;

	case WM_SYSCHAR:
		toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
		break;

	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wparam == VK_F11) {
			is_restarting = true;
			TestShutdown();
			TestInitialize();
		}
		break;
	}

	if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen) {
		Platform::Window win{ Platform::WindowID{ (ID::ID_Type)GetWindowLongPtr(hwnd, GWLP_USERDATA) } };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (win.GetID() == _surfaces[i].window.GetID()) {
				if (toggle_fullscreen) {
					win.SetFullScreen(!win.IsFullscreen());
					return 0;
				}
				else {
					_surfaces[i].surface.Resize(win.Width(), win.Height());
					resized = false;
				}
				break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

GameEntity::Entity CreateGameEntity() {
	Transform::InitInfo transform_info{};
	Math::v3a rot{ 0, 3.14f, 0 };
	DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot))};
	Math::v4a rot_quat;
	DirectX::XMStoreFloat4A(&rot_quat, quat);
	memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));

	GameEntity::EntityInfo entity_info{};
	entity_info.transform = &transform_info;
	GameEntity::Entity ntt{ GameEntity::CreateGameEntity(entity_info) };
	assert(ntt.IsValid());
	return ntt;
}

void CreateRenderSurface(Graphics::RenderSurface& surface, Platform::WindowInitInfo& info) {
	surface.window = Platform::CreateWindow(&info);
	surface.surface = Graphics::CreateSurface(surface.window);
}

void DestroyRenderSurface(Graphics::RenderSurface& surface) {
	Graphics::RenderSurface temp{ surface };
	surface = {};
	if(temp.surface.IsValid()) Graphics::RemoveSurface(temp.surface.GetID());
	if(temp.window.IsValid()) Platform::RemoveWindow(temp.window.GetID());
}

bool ReadFile(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size) {
	if (!std::filesystem::exists(path)) return false;

	size = std::filesystem::file_size(path);
	assert(size);
	if (!size) return false;
	data = std::make_unique<u8[]>(size);
	std::ifstream file{ path, std::ios::in | std::ios::binary };
	if (!file || !file.read((char*)data.get(), size)) { file.close(); return false; }

	file.close();
	return true;
}

bool TestInitialize() {
	while (!CompileShaders()) {
		if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
			return false;
	}

	if (!Graphics::Initialize(Graphics::GraphicsPlatform::Direct3D12)) return false;
	Platform::WindowInitInfo info[]{
		{&WinProc, nullptr, L"Test Window", 500, 100, 400, 800},
		{&WinProc, nullptr, L"Test Window", 550, 150, 800, 400},
		{&WinProc, nullptr, L"Test Window", 600, 200, 400, 400},
		{&WinProc, nullptr, L"Test Window", 650, 250, 800, 600},
	};

	static_assert(_countof(info) == _countof(_surfaces));

	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		CreateRenderSurface(_surfaces[i], info[i]);

	std::unique_ptr<u8[]> model;
	u64 size{ 0 };
	if (!ReadFile("..\\..\\EngineTest\\model.model", model, size)) return false;

	model_id = Content::CreateResource(model.get(), Content::AssetType::Mesh);
	if (!ID::IsValid(model_id)) return false;

	InitTestWorkers(BufferTestWorker);

	camera.entity = CreateGameEntity();
	camera.camera = Graphics::CreateCamera(Graphics::PerspectiveCameraInitInfo(camera.entity.GetID()));
	assert(camera.camera.IsValid());

	item_id = CreateRenderItem(CreateGameEntity().GetID());

	is_restarting = false;
	return true;
}

void TestShutdown() {
	DestroyRenderItem(item_id);

	if (camera.camera.IsValid()) Graphics::RemoveCamera(camera.camera.GetID());
	if (camera.entity.IsValid()) GameEntity::RemoveGameEntity(camera.entity.GetID());

	JoinTestWorkers();

	if (ID::IsValid(model_id)) Content::DestroyResource(model_id, Content::AssetType::Mesh);

	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		DestroyRenderSurface(_surfaces[i]);

	Graphics::Shutdown();
}

bool EngineTest::Initialize()  {
	return TestInitialize();
}

void EngineTest::Run()  {
	timer.Begin();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		if(_surfaces[i].surface.IsValid())
			_surfaces[i].surface.Render();
	timer.End();
}

void EngineTest::Shutdown()  {
	TestShutdown();
}

#endif