#pragma once
#include "WickedEngine.h"

class Tests : public MainComponent
{
public:
	Tests();
	virtual ~Tests();

	virtual void Initialize() override;
};


class TestsRenderer : public DeferredRenderableComponent
{
public: 
	TestsRenderer();
	virtual ~TestsRenderer();
};

