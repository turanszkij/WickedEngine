#ifndef WICKED_ENGINE
#define WICKED_ENGINE

#include "CommonInclude.h"

#include "wiVersion.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiBackLog.h"
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
#include "wiRawInput.h"
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
#include "wiEnums.h"
#include "wiInitializer.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiGraphicsAPI.h"
#include "wiGUI.h"
#include "wiWidget.h"
#include "wiHashString.h"
#include "wiWindowRegistration.h"
#include "wiTranslator.h"
#include "wiArchive.h"
#include "wiSpinLock.h"

#include "RenderableComponent.h"
#include "Renderable2DComponent.h"
#include "Renderable3DComponent.h"
#include "ForwardRenderableComponent.h"
#include "DeferredRenderableComponent.h"
#include "TiledForwardRenderableComponent.h"
#include "LoadingScreenComponent.h"
#include "MainComponent.h"


#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"

#ifdef _WIN32

#ifdef WINSTORE_SUPPORT
#pragma comment(lib,"WickedEngine_UWP.lib")
#else
#pragma comment(lib,"WickedEngine_Windows.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"pdh.lib")
#endif // WINSTORE_SUPPORT

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"dxguid.lib")

#endif // _WIN32



#endif // WICKED_ENGINE
