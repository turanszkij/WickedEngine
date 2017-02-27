#include "wiInitializer.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiLensFlare.h"
#include "wiResourceManager.h"
#include "wiFrameRate.h"
#include "wiBackLog.h"
#include "wiCpuInfo.h"
#include "wiSound.h"
#include "wiHelper.h"


namespace wiInitializer
{

	void InitializeComponents(int requestedFeautures)
	{
		if (requestedFeautures & WICKEDENGINE_INITIALIZE_MISC)
		{
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
			wiFont::Initialize();
			wiFont::SetUpStaticComponents();
		}

		if (requestedFeautures & WICKEDENGINE_INITIALIZE_SOUND)
		{
			if (FAILED(wiSoundEffect::Initialize()) || FAILED(wiMusic::Initialize()))
			{
				stringstream ss("");
				ss << "Failed to Initialize Audio Device!";
				wiHelper::messageBox(ss.str());
			}
		}

	}
}