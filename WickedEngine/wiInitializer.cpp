#include "wiInitializer.h"
#include "WickedEngine.h"

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

		wiBackLog::post("Initializing Wicked Engine, please wait...\n");

		wiFont::Initialize();
		wiImage::Initialize();
		wiRenderer::Initialize();
		wiSceneSystem::wiHairParticle::Initialize();
		wiSceneSystem::wiEmittedParticle::Initialize();
		wiCpuInfo::Initialize();
		wiLensFlare::Initialize();
		wiOcean::Initialize();
		wiWidget::LoadShaders();
		wiGPUSortLib::LoadShaders();
		wiGPUBVH::LoadShaders();
		wiPhysicsEngine::Initialize();
		wiSoundEffect::Initialize();
		wiMusic::Initialize();

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