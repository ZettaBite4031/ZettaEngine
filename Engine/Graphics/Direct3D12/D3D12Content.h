#pragma once
#include "D3D12CommonHeaders.h"

namespace Zetta::Graphics::D3D12::Content {
	namespace Submesh{
		ID::ID_Type Add(const u8*& data);
		void Remove(ID::ID_Type id);
	}
}