#ifndef _VOLUMELIGHT_HF_
#define _VOLUMELIGHT_HF_
#include "globals.hlsli"

struct VertexToPixel{
	float4 pos			: SV_POSITION;
	float4 col			: COLOR;
};

#endif // _VOLUMELIGHT_HF_