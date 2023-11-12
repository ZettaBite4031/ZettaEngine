#pragma once
#include "CommonHeaders.h"

namespace Zetta::Math {
	constexpr float PI = 3.1415926535897932384626433832795f;
	constexpr float EPSILON = 1e-10f;
#if defined(_WIN64)
	using v2 = DirectX::XMFLOAT2;
	using v2a = DirectX::XMFLOAT2A;
	using v3 = DirectX::XMFLOAT3;
	using v3a = DirectX::XMFLOAT3A;
	using v4 = DirectX::XMFLOAT4;
	using v4a = DirectX::XMFLOAT4A;
	using u32v2 = DirectX::XMUINT2;
	using u32v3 = DirectX::XMUINT3;
	using u32v4 = DirectX::XMUINT4;
	using s32v2 = DirectX::XMINT2;
	using s32v3 = DirectX::XMINT3;
	using s32v4 = DirectX::XMINT4;
	using mat3 = DirectX::XMFLOAT3X3;
	using mat4 = DirectX::XMFLOAT4X4;
	using mat4a = DirectX::XMFLOAT4X4A;

#endif
}