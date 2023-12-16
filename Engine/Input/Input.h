#pragma once
#include "CommonHeaders.h"
#include "EngineAPI/Input.h"

namespace Zetta::Input {
	void Bind(InputSource source);
	void Unbind(InputSource::Type type, InputCode::Code code);
	void Unbind(u64 binding);
	void Set(InputSource::Type type, InputCode::Code code, Math::v3 value);
}