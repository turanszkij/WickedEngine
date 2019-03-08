#pragma once
#include "WickedEngine.h"

class Tests : public MainComponent
{
public:
	virtual void Initialize() override;
};


class TestsRenderer : public RenderPath3D_Deferred
{
public: 
	TestsRenderer();

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
};

