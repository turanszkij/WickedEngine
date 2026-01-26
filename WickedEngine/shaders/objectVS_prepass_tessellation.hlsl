#define OBJECTSHADER_COMPILE_VS
#define OBJECTSHADER_LAYOUT_PREPASS_TEX // Remember: tessellation always needs uvs for displacement mappig even without alphatest
#define OBJECTSHADER_USE_NOCAMERA
#define OBJECTSHADER_USE_NORMAL
#include "objectHF.hlsli"
