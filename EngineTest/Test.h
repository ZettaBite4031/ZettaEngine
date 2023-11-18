#pragma once
#include <thread>
#include <ctime>
#include <string>

#define TEST_ENTITY_COMPONENTS 0
#define TEST_WINDOW 0
#define TEST_RENDERER 1

class Test {
public:
	virtual bool Initialize() = 0;
	virtual void Run() = 0;
	virtual void Shutdown() = 0;
};

#if _WIN64
#include <Windows.h>
class TimeIt {
public:
	using Clock = std::chrono::high_resolution_clock;
	using TimeStamp = std::chrono::steady_clock::time_point;

	void Begin() {
		_start = Clock::now();
	}

	void End() {
		auto dt = Clock::now() - _start;
		_ms_avg += ((float)std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() - _ms_avg) / (float)_counter;
		_counter++;

		if (std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - _seconds).count() >= 1) {
			OutputDebugStringA("Avg. frame (ms): ");
			OutputDebugStringA(std::to_string(_ms_avg).c_str());
			OutputDebugStringA((" " + std::to_string(_counter)).c_str());
			OutputDebugStringA(" fps\n");
			_ms_avg = 0.f;
			_counter = 1;
			_seconds = Clock::now();
		}
	}

private:
	float _ms_avg{ 0.f };
	int _counter{ 1 };
	TimeStamp _start;
	TimeStamp _seconds{ Clock::now() };
};
#endif