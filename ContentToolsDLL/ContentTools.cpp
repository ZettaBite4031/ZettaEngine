#include "ToolsCommon.h"

namespace Zetta::Tools {
	extern void ShutdownTextureTools();
}

EDITOR_INTERFACE void ShutdownContentTools() {
	using namespace Zetta::Tools;
	ShutdownTextureTools();
}
