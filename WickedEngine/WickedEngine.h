#ifndef WICKED_ENGINE_VERSION
#define WICKED_ENGINE_VERSION "1.0"

#include <SDKDDKVer.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <Xinput.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <stack>
#include <deque>
#include <queue>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <wincodec.h>
#include <iomanip>
#include <float.h>
#include <functional>

using namespace DirectX;
using namespace std;


#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"
#include "RenderTarget.h"
#include "BackLog.h"
#include "Camera.h"
#include "Frustum.h"
#include "ImageEffects.h"
#include "Image.h"
#include "Font.h"
#include "FrameRate.h"
#include "CpuInfo.h"
#include "WickedLoader.h"
#include "Particle.h"
#include "Renderer.h"
#include "DirectInput.h"
#include "XInput.h"
#include "TaskThread.h"
#include "mysql/mysql.h"
#include "WickedMath.h"
#include "LensFlare.h"
#include "Sound.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "WickedHelper.h"
#include "InputManager.h"

////#include "ViewProp.h"



#define XBOUNDS 10
#define CAMERA_POSITIONY 4.0f
#define DEFAULT_CAMERADISTANCE -9.5f
#define MODIFIED_CAMERADISTANCE -11.5f
#define CAMERA_DISTANCE_MODIFIER 10
#define STARTPOSITION 4.5




enum POVENUM{ //NUMPAD SORTING
	D_UNUSED,
	D_DOWNLEFT,
	D_DOWN,
	D_RIGHTDOWN,
	D_LEFT,
	D_IDLE, //5
	D_RIGHT,
	D_LEFTUP,
	D_UP,
	D_UPRIGHT,
};


#define SETTINGSFILE "settings/ProgramSettings.ini"

#pragma comment(lib,"WickedEngine.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"lib32/libmysql.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"pdh.lib")


#endif
