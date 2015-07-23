#include "wiInitializer.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiLensFlare.h"
#include "wiResourceManager.h"
#include "wiFrameRate.h"
#include "wiBackLog.h"
#include "wiCpuInfo.h"
#include "wiSound.h"


namespace wiInitializer
{

	void InitializeComponents(int requestedFeautures)
	{
		if (requestedFeautures & WICKEDENGINE_INITIALIZE_MISC)
		{
			wiResourceManager::SetUp();
			wiBackLog::Initialize();
			wiFrameRate::Initialize();
			wiCpuInfo::Initialize();
		}

		if (requestedFeautures & WICKEDENGINE_INITIALIZE_RENDERER)
		{
			wiRenderer::SetUpStaticComponents();
			wiLensFlare::Initialize();
		}

		if (requestedFeautures & WICKEDENGINE_INITIALIZE_IMAGE)
		{
			wiImage::Load();
		}

		if (requestedFeautures & WICKEDENGINE_INITIALIZE_FONT)
		{
			wiFont::SetUpStaticComponents();
		}

		if (requestedFeautures & WICKEDENGINE_INITIALIZE_SOUND)
		{
			wiSoundEffect::Initialize();
			wiMusic::Initialize();
		}

	}
}