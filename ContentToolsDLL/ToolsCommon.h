#pragma once
#include "CommonHeaders.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <wrl.h>

#ifndef EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif

class Progression {
public: 
	using ProgressCallback = void(*)(s32, s32);

	Progression() = default;
	explicit Progression(ProgressCallback callback)
		: m_Callback{ callback } {}

	DISABLE_COPY(Progression);

	void Callback(u32 value, u32 maximum) {
		m_Value = value;
		m_Maximum = maximum;
		if (m_Callback) m_Callback(value, maximum);
	}

	[[nodiscard]] constexpr u32 Maximum() const { return m_Maximum; }
	[[nodiscard]] constexpr u32 Value() const { return m_Value; }

private:
	ProgressCallback m_Callback;
	u32				 m_Value;
	u32				 m_Maximum;
};

inline bool FileExists(const char* file) {
	const DWORD attr{ GetFileAttributesA(file) };
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

inline std::wstring ToWString(const char* cstr) {
	std::string s{ cstr };
	return { s.begin(), s.end() };
}

inline Zetta::util::vector<std::string> split(std::string s, char delimiter) {
	size_t start{ 0 };
	size_t end{ 0 };
	std::string substring;
	Zetta::util::vector<std::string> strings;

	while ((end = s.find(delimiter, start)) != std::string::npos) {
		substring = s.substr(start, end - start);
		start = end + sizeof(char);
		strings.emplace_back(substring);
	}

	strings.emplace_back(s.substr(start));
	return strings;
}