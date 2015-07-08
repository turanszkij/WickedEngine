#ifndef WICKED_ENGINE_VERSION
#define WICKED_ENGINE_VERSION "1.01"

#include "CommonInclude.h"

#ifndef WINSTORE_SUPPORT
#include "mysql/mysql.h"
#endif
#include "RenderTarget.h"
#include "DepthTarget.h"
#include "BackLog.h"
#include "Camera.h"
#include "Frustum.h"
#include "ImageEffects.h"
#include "Image.h"
#include "Sprite.h"
#include "Font.h"
#include "FrameRate.h"
#include "CpuInfo.h"
#include "WickedLoader.h"
#include "Particle.h"
#include "Renderer.h"
#include "DirectInput.h"
#include "XInput.h"
#include "TaskThread.h"
#include "WickedMath.h"
#include "LensFlare.h"
#include "Sound.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "WickedHelper.h"
#include "InputManager.h"

#include "RenderableComponent.h"
#include "Renderable3DSceneComponent.h"
#include "ForwardRenderableComponent.h"
#include "DeferredRenderableComponent.h"




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
