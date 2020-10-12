#ifndef WICKED_ENGINE
#define WICKED_ENGINE

// NOTE:
// The purpose of this file is to expose all engine features.
// It should be included in the engine's implementing project not the engine itself!
// It should be included in the precompiled header if available.

#include "CommonInclude.h"

// High-level interface:
#include "RenderPath.h"
#include "RenderPath2D.h"
#include "RenderPath3D.h"
#include "RenderPath3D_PathTracing.h"
#include "LoadingScreen.h"
#include "MainComponent.h"

// Engine-level systems
#include "wiVersion.h"
#include "wiPlatform.h"
#include "wiBackLog.h"
#include "wiIntersect.h"
#include "wiImage.h"
#include "wiFont.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiScene.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiMath.h"
#include "wiAudio.h"
#include "wiResourceManager.h"
#include "wiTimer.h"
#include "wiHelper.h"
#include "wiInput.h"
#include "wiRawInput.h"
#include "wiXInput.h"
#include "wiTextureHelper.h"
#include "wiRandom.h"
#include "wiColor.h"
#include "wiPhysicsEngine.h"
#include "wiEnums.h"
#include "wiInitializer.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiGraphicsDevice.h"
#include "wiGUI.h"
#include "wiWidget.h"
#include "wiArchive.h"
#include "wiSpinLock.h"
#include "wiRectPacker.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "wiStartupArguments.h"
#include "wiGPUBVH.h"
#include "wiGPUSortLib.h"
#include "wiJobSystem.h"
#include "wiNetwork.h"
#include "wiEvent.h"

#ifdef _WIN32
#ifdef PLATFORM_UWP
#pragma comment(lib,"WickedEngine_UWP.lib")
#else
#pragma comment(lib,"WickedEngine_Windows.lib")
#endif // PLATFORM_UWP
#endif // _WIN32



#endif // WICKED_ENGINE
