#pragma once
#include "WickedEngine.h"


class TestsRenderer : public RenderPath3D_Deferred
{
public:
	void Load() override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
};

class Tests : public MainComponent
{
	TestsRenderer renderer;
public:
	virtual void Initialize() override;
};

