#pragma once
#include "WickedEngine.h"

class Tests : public MainComponent
{
public:
	Tests();
	virtual ~Tests();

	virtual void Initialize() override;
};


class TestsRenderer : public RenderPath3D_Deferred
{
public: 
	TestsRenderer();
	virtual ~TestsRenderer();


	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
};

