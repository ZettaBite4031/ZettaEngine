#pragma once
#include "CommonHeaders.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <wrl.h>

#ifndef EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif

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