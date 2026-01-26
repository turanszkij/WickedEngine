#define OBJECTSHADER_COMPILE_DS
#define OBJECTSHADER_LAYOUT_SHADOW_TEX // Remember: tessellation always needs uvs for displacement mappig even without alphatest
#define OBJECTSHADER_USE_COLOR
#define OBJECTSHADER_USE_NORMAL
#include "objectHF_tessellation.hlsli"

