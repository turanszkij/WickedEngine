#ifndef WICKEDENGINE_COMMONINCLUDE_H
#define WICKEDENGINE_COMMONINCLUDE_H

#include <SDKDDKVer.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DXGI1_2.h>
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

#ifdef WINSTORE_SUPPORT
#include <Windows.UI.Core.h>
#endif

using namespace DirectX;
using namespace std;


#include "Utility/WicTextureLoader.h"
#include "Utility/DDSTextureLoader.h"
#include "Utility/ScreenGrab.h"


//TODO: REMOVE!!!
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



#endif //WICKEDENGINE_COMMONINCLUDE_H
