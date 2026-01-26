#define OBJECTSHADER_COMPILE_PS
#define OBJECTSHADER_LAYOUT_PREPASS_TEX // Remember: tessellation always needs uvs for displacement mappig even without alphatest
#define OBJECTSHADER_USE_NORMAL
#define PREPASS
#define DISABLE_ALPHATEST
#include "objectHF.hlsli"

