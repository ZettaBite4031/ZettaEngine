#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "EngineAPI/Input.h"

using namespace Zetta;

class RotatorScript;
REGISTER_SCRIPT(RotatorScript);
class RotatorScript : public Script::EntityScript {
public:
	constexpr explicit RotatorScript(GameEntity::Entity entity)
		: Script::EntityScript{ entity } {}

	void OnWake() override {}

	void Update(float dt) override {
		_angle += 0.25f * dt * Math::TAU;
		if (_angle > Math::TAU) _angle -= Math::TAU;
		Math::v3a rot{ 0.f, _angle, 0.f };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
		Math::v4 rot_quat{};
		DirectX::XMStoreFloat4(&rot_quat, quat);
		SetRotation(rot_quat);
	}

private:
	f32 _angle{ 0.f };
};

class FanScript;
REGISTER_SCRIPT(FanScript);
class FanScript : public Script::EntityScript {
public:
	constexpr explicit FanScript(GameEntity::Entity entity)
		: Script::EntityScript{ entity } {}

	void OnWake() override {}

	void Update(float dt) override {
		_angle -= 0.25f * dt * Math::TAU;
		if (_angle < 0.f) _angle += Math::TAU;
		Math::v3a rot{ _angle, 0.f, 0.f };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
		Math::v4 rot_quat{};
		DirectX::XMStoreFloat4(&rot_quat, quat);
		SetRotation(rot_quat);
	}

private:
	f32 _angle{ 0.f };
};

class ShipScript;
REGISTER_SCRIPT(ShipScript);
class ShipScript : public Script::EntityScript {
public:
	constexpr explicit ShipScript(GameEntity::Entity entity)
		: Script::EntityScript{ entity } {}

	void OnWake() override {}

	void Update(float dt) override {
		_angle -= 0.01f * dt * Math::TAU;
		if (_angle < 0.f) _angle += Math::TAU;
		f32 x{ _angle * 2.f - Math::PI };
		const f32 s1{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };
		x = _angle;
		const f32 s2{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };

		Math::v3a rot{ s1, 0.f, s2 };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
		Math::v4 rot_quat{};
		DirectX::XMStoreFloat4(&rot_quat, quat);
		SetRotation(rot_quat);
		Math::v3 pos{ Position() };
		pos.y = 1.3f + 0.2f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x));
		SetPosition(pos);
	}

private:
	f32 _angle{ 0.f };
};

class CameraScript;
REGISTER_SCRIPT(CameraScript);
class CameraScript : public Script::EntityScript {
public:
	explicit CameraScript(GameEntity::Entity entity)
		: Script::EntityScript{ entity }
	{
		_input_system.AddHandler(Input::InputSource::Mouse, this, &CameraScript::MouseMove);

		static u64 binding{ std::hash<std::string>()("move") };
		_input_system.AddHandler(binding, this, &CameraScript::OnMove);

		Math::v3 pos{ Position() };
		_desired_position = _position = DirectX::XMLoadFloat3(&pos);

		Math::v3 dir{ Orientation() };
		f32 theta{ DirectX::XMScalarACos(dir.y) };
		f32 phi{ std::atan2(-dir.z, dir.x) };
		Math::v3 rot{ theta - Math::hPI, phi + Math::hPI, 0.f };
		_desired_spherical = _spherical = DirectX::XMLoadFloat3(&rot);
	}

	void OnWake() override {}
	void Update(float dt) override {
		using namespace DirectX;

		if (_move_magnitude > Math::EPSILON) {
			const f32 fps_scale{ dt / 0.0166667f };
			Math::v4 rot{ Rotation() };
			DirectX::XMVECTOR d{ DirectX::XMVector3Rotate(_move * 0.25f * fps_scale, XMLoadFloat4(&rot)) };
			if (_position_acceleration < 1.f) _position_acceleration += (0.05f * fps_scale);
			_desired_position += (d * _position_acceleration);
			_move_position = true;
		}
		else if (_move_position) {
			_position_acceleration = 0.0f;
		}

		if (_move_rotation || _move_position) SeekCamera(dt);
	}

private:
	void OnMove(u64 binding, const Input::InputValue& value) {
		using namespace DirectX;
		_move = XMLoadFloat3(&value.current);
		_move_magnitude = XMVectorGetX(XMVector3LengthSq(_move));
	}

	void MouseMove(Input::InputSource::Type type, Input::InputCode::Code code, const Input::InputValue& mouse_pos) {
		if (code == Input::InputCode::MousePosition) {
			Input::InputValue value;
			Input::Get(Input::InputSource::Mouse, Input::InputCode::MouseLeft, value);
			if (value.current.z == 0.f) return;

			const f32 scale{ 0.005f };
			const f32 dx{ (mouse_pos.current.x - mouse_pos.previous.x) * scale };
			const f32 dy{ (mouse_pos.current.y - mouse_pos.previous.y) * scale };
			
			Math::v3 spherical;
			DirectX::XMStoreFloat3(&spherical, _desired_spherical);
			spherical.x += dy;
			spherical.y -= dx;
			spherical.x = Math::clamp(spherical.x, 0.0001f - Math::hPI, Math::hPI - 0.0001f);

			_desired_spherical = DirectX::XMLoadFloat3(&spherical);
			_move_rotation = true;
		}
	}

	void SeekCamera(f32 dt) {
		using namespace DirectX;
		XMVECTOR p{ _desired_position - _position };
		XMVECTOR o{ _desired_spherical - _spherical };
	
		_move_position = (XMVectorGetX(XMVector3LengthSq(p)) > Math::EPSILON);
		_move_rotation = (XMVectorGetX(XMVector3LengthSq(o)) > Math::EPSILON);

		const f32 scale{ 0.5f * dt / 0.01666667f };
		
		if (_move_position) {
			_position += (p * scale);
			Math::v3 new_pos;
			XMStoreFloat3(&new_pos, _position);
			SetPosition(new_pos);
		}

		if (_move_rotation) {
			_spherical += (o * scale);
			Math::v3 new_rot;
			XMStoreFloat3(&new_rot, _spherical);
			new_rot.x = Math::clamp(new_rot.x, 0.0001f - Math::hPI, Math::hPI - 0.0001f);
			_spherical = DirectX::XMLoadFloat3(&new_rot);

			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(_spherical) };
			Math::v4 rot_quat;
			XMStoreFloat4(&rot_quat, quat);
			SetRotation(rot_quat);
		}
	}

	Input::InputSystem<CameraScript> _input_system{};

	DirectX::XMVECTOR _desired_spherical;
	DirectX::XMVECTOR _spherical;
	DirectX::XMVECTOR _desired_position;
	f32 _position_acceleration{ 0.f };
	f32 _move_magnitude{ 0.f };
	DirectX::XMVECTOR _move{};
	DirectX::XMVECTOR _position;
	bool _move_rotation{ false };
	bool _move_position{ false };
};
