#pragma once
#include "Test.h"


class EngineTest : public Test {
public:
	virtual bool Initialize() override;
	virtual void Run() override;
	virtual void Shutdown() override;
};