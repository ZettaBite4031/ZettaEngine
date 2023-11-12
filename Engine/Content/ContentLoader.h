#pragma once
#include "../Common/CommonHeaders.h"


#if !defined(SHIPPING)
namespace Zetta::Content {
	bool LoadGame();
	void UnloadGame();
}
#endif