#pragma once
#include "Common/CommonHeaders.h"


#if !defined(SHIPPING)
namespace Zetta::Content {
	bool LoadGame();
	void UnloadGame();

	bool LoadEngineShaders(std::unique_ptr<u8[]>& shaders, u64& size);
}
#endif