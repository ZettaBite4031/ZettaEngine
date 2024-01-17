#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Graphics/Direct3D12/D3D12Core.h"
#include "Components/Entity.h"
#include "Components/Script.h"
#include "Components/Transform.h"
#include "Input/Input.h"
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

struct CameraSurface {
	GameEntity::Entity entity{};
	Graphics::Camera camera{};
	Graphics::RenderSurface surface{};
};

ID::ID_Type model_id{ ID::Invalid_ID };
ID::ID_Type item_id{ ID::Invalid_ID };

CameraSurface _surfaces[4];
TimeIt timer{};

bool resized{ false };
bool is_restarting{ false };
void DestroyCameraSurface(CameraSurface& surface);

bool TestInitialize();
void TestShutdown();

void CreateRenderItems();
void DestroyRenderItems();

void GenerateLights();
void RemoveLights();
void TestLights(f32);


void GetRenderItems(ID::ID_Type* items, u32 count);

LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	bool toggle_fullscreen{ false };

	switch (msg) {
	case WM_DESTROY:
	{
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (_surfaces[i].surface.window.IsValid()) {
				if (_surfaces[i].surface.window.IsClosed()) DestroyCameraSurface(_surfaces[i]);
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

	if ((resized && GetKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen) {
		Platform::Window win{ Platform::WindowID{ (ID::ID_Type)GetWindowLongPtr(hwnd, GWLP_USERDATA) } };
		for (u32 i{ 0 }; i < _countof(_surfaces); i++) {
			if (win.GetID() == _surfaces[i].surface.window.GetID()) {
				if (toggle_fullscreen) {
					win.SetFullScreen(!win.IsFullscreen());
					return 0;
				}
				else {
					_surfaces[i].surface.surface.Resize(win.Width(), win.Height());
					_surfaces[i].camera.AspectRatio((f32)win.Width() / win.Height());
					resized = false;
				}
				break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

GameEntity::Entity CreateGameEntity(Math::v3 position, Math::v3 rotation, const char* script_name) {
	Transform::InitInfo transform_info{};
	DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&rotation))};
	Math::v4a rot_quat;
	DirectX::XMStoreFloat4A(&rot_quat, quat);
	memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));
	memcpy(&transform_info.position[0], &position.x, sizeof(transform_info.position));

	Script::InitInfo script_info{};
	if (script_name) {
		script_info.script_creator = Script::Detail::GetScriptCreatorInternal(Script::Detail::StringHash()(script_name));
		assert(script_info.script_creator);
	}

	GameEntity::EntityInfo entity_info{};
	entity_info.transform = &transform_info;
	entity_info.script = &script_info;
	GameEntity::Entity ntt{ GameEntity::CreateGameEntity(entity_info) };
	assert(ntt.IsValid());
	return ntt;
}

void RemoveGameEntity(GameEntity::EntityID id) {
	GameEntity::RemoveGameEntity(id);
}

void CreateCameraSurface(CameraSurface& surface, Platform::WindowInitInfo& info) {
	surface.surface.window = Platform::CreateWindow(&info);
	surface.surface.surface = Graphics::CreateSurface(surface.surface.window);
	surface.entity = CreateGameEntity({ 13.76f, 3.f, -1.1f }, { -0.117f, -2.1f, 0.f }, "CameraScript");
	surface.camera = Graphics::CreateCamera(Graphics::PerspectiveCameraInitInfo{ surface.entity.GetID() });
	surface.camera.AspectRatio((f32)surface.surface.window.Width() / surface.surface.window.Height());
}

void DestroyCameraSurface(CameraSurface& surface) {
	CameraSurface temp{ surface };
	surface = {};
	if (temp.surface.surface.IsValid()) Graphics::RemoveSurface(temp.surface.surface.GetID());
	if (temp.surface.window.IsValid()) Platform::RemoveWindow(temp.surface.window.GetID());
	if (temp.camera.IsValid()) Graphics::RemoveCamera(temp.camera.GetID());
	if (temp.entity.IsValid()) GameEntity::RemoveGameEntity(temp.entity.GetID());
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
	while (!CompileShadersSM66()) {
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
		CreateCameraSurface(_surfaces[i], info[i]);

	std::unique_ptr<u8[]> model;
	u64 size{ 0 };
	if (!ReadFile("..\\..\\EngineTest\\model.model", model, size)) return false;

	model_id = Content::CreateResource(model.get(), Content::AssetType::Mesh);
	if (!ID::IsValid(model_id)) return false;

	InitTestWorkers(BufferTestWorker);

	CreateRenderItems();

	GenerateLights();

	Input::InputSource source{};
	source.binding = std::hash<std::string>()("move");
	source.source_type = Input::InputSource::Keyboard;
	source.code = Input::InputCode::KeyA;
	source.multiplier = 1.f;
	source.axis = Input::Axis::X;
	Input::Bind(source);

	source.code = Input::InputCode::KeyD;
	source.multiplier = -1.f;
	Input::Bind(source);

	source.code = Input::InputCode::KeyW;
	source.multiplier = 1.f;
	source.axis = Input::Axis::Z;
	Input::Bind(source);

	source.code = Input::InputCode::KeyS;
	source.multiplier = -1.f;
	Input::Bind(source);

	source.code = Input::InputCode::KeySpace;
	source.multiplier = 1.f;
	source.axis = Input::Axis::Y;
	Input::Bind(source);

	source.code = Input::InputCode::KeyLeftControl;
	source.multiplier = -1.f;
	Input::Bind(source);

	is_restarting = false;
	return true;
}

void TestShutdown() {
	Input::Unbind(std::hash<std::string>()("move"));
	RemoveLights();

	DestroyRenderItems();

	JoinTestWorkers();

	if (ID::IsValid(model_id)) Content::DestroyResource(model_id, Content::AssetType::Mesh);

	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		DestroyCameraSurface(_surfaces[i]);

	Graphics::Shutdown();
}

bool EngineTest::Initialize()  {
	return TestInitialize();
}

void EngineTest::Run()  {
	static u32 counter{ 0 };
	static u32 light_set_key{ 0 };
	counter++;
	//if ((counter % 90) == 0) light_set_key = (light_set_key + 1) % 2;

	timer.Begin();
	//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	const f32 dt{ timer.AvgDT() };
	Script::Update(dt);
	//TestLights(dt);
	for (u32 i{ 0 }; i < _countof(_surfaces); i++)
		if (_surfaces[i].surface.surface.IsValid()) {
			f32 thresholds[3]{ };

			ID::ID_Type render_items[3]{};
			GetRenderItems(&render_items[0], 3);

			Graphics::FrameInfo info{};
			info.render_item_ids = &render_items[0];
			info.render_item_count = 3;
			info.thresholds = &thresholds[0];
			info.average_frame_time = dt;
			info.light_set_key = light_set_key;
			info.camera_id = _surfaces[i].camera.GetID();

			assert(_countof(thresholds) >= info.render_item_count);
			_surfaces[i].surface.surface.Render(info);
		}
	timer.End();
}

void EngineTest::Shutdown()  {
	TestShutdown();
}

#endif