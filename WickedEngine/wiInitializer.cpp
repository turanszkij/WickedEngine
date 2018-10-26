#include "wiInitializer.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiLensFlare.h"
#include "wiResourceManager.h"
#include "wiFrameRate.h"
#include "wiBackLog.h"
#include "wiCpuInfo.h"
#include "wiSound.h"
#include "wiOcean.h"
#include "wiHelper.h"
#include "wiWidget.h"
#include "wiGPUSortLib.h"
#include "wiGPUBVH.h"
#include "wiPhysicsEngine.h"

#include <thread>
#include <sstream>

using namespace std;

namespace wiInitializer
{
	static bool finished = false;
	static float initializationTime = 0;

	void InitializeComponentsImmediate()
	{
		if (finished)
		{
			return;
		}

		wiTimer timer;
		timer.record();

		wiFont::Initialize();
		wiImage::Initialize();

		wiBackLog::Initialize();
		wiFrameRate::Initialize();
		wiCpuInfo::Initialize();

		wiRenderer::Initialize();
		wiLensFlare::Initialize();
		wiOcean::Initialize();

		wiWidget::LoadShaders();
		wiGPUSortLib::LoadShaders();
		wiGPUBVH::LoadShaders();

		wiPhysicsEngine::Initialize();

		if (FAILED(wiSoundEffect::Initialize()) || FAILED(wiMusic::Initialize()))
		{
			stringstream ss("");
			ss << "Failed to Initialize Audio Device!";
			wiHelper::messageBox(ss.str());
		}

		initializationTime = (float)(timer.elapsed() / 1000.0);

		finished = true;

	}
	void InitializeComponentsAsync()
	{
		if (finished)
		{
			return;
		}

		std::thread([] {

			InitializeComponentsImmediate();

		}).detach();

	}

	bool IsInitializeFinished()
	{
		return finished;
	}

	float GetInitializationTimeInSeconds()
	{
		return initializationTime;
	}
}