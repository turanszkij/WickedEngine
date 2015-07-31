#ifndef WICKED_ENGINE_VERSION
#define WICKED_ENGINE_VERSION "1.01"

#include "CommonInclude.h"

#ifndef WINSTORE_SUPPORT
#include "mysql/mysql.h"
#endif
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiBackLog.h"
#include "wiCamera.h"
#include "wiFrustum.h"
#include "wiImageEffects.h"
#include "wiImage.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiFrameRate.h"
#include "wiCpuInfo.h"
#include "wiLoader.h"
#include "wiParticle.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiDirectInput.h"
#include "wiXInput.h"
#include "wiTaskThread.h"
#include "wiMath.h"
#include "wiLensFlare.h"
#include "wiSound.h"
#include "wiThreadSafeManager.h"
#include "wiResourceManager.h"
#include "wiTimer.h"
#include "wiHelper.h"
#include "wiInputManager.h"
#include "wiCVars.h"
#include "wiTextureHelper.h"
#include "wiRandom.h"
#include "wiColor.h"
#include "wiWaterPlane.h"
#include "wiPHYSICS.h"
#include "wiBULLET.h"
#include "wiGraphicsThreads.h"
#include "wiStencilRef.h"
#include "wiInitializer.h"

#include "RenderableComponent.h"
#include "Renderable2DComponent.h"
#include "Renderable3DComponent.h"
#include "ForwardRenderableComponent.h"
#include "DeferredRenderableComponent.h"
#include "LoadingScreenComponent.h"
#include "MainComponent.h"


#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"



#ifndef WINSTORE_SUPPORT
#pragma comment(lib,"lib32/libmysql.lib")
#endif
#pragma comment(lib,"WickedEngine.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"pdh.lib")


#endif
