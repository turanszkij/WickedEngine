#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"

// TODO: bring the post process settings here from wiImageParams!

struct wiPostProcessParams;

namespace wiPostProcess
{
	void Initialize();
	void Cleanup();
	void LoadShaders();

	void Draw(const wiPostProcessParams& params, GRAPHICSTHREAD threadID);
}

struct wiPostProcessParams
{

};
