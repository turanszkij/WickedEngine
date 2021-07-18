#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#define A_GPU 1
#define A_HLSL 1
#include "ffx-fsr/ffx_a.h"

static const uint4 Sample = 0;

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

AF4 FsrEasuRF(AF2 p) { AF4 res = input.GatherRed(sampler_linear_clamp, p, int2(0, 0)); return res; }
AF4 FsrEasuGF(AF2 p) { AF4 res = input.GatherGreen(sampler_linear_clamp, p, int2(0, 0)); return res; }
AF4 FsrEasuBF(AF2 p) { AF4 res = input.GatherBlue(sampler_linear_clamp, p, int2(0, 0)); return res; }

#define FSR_EASU_F 1
#include "ffx-fsr/ffx_fsr1.h"

void CurrFilter(int2 pos)
{
	AF3 c;
	FsrEasuF(c, pos, xFSR_Const0, xFSR_Const1, xFSR_Const2, xFSR_Const3);
	if (Sample.x == 1)
		c *= c;
	output[pos] = float4(c, 1);
}

[numthreads(64, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
	// Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
	AU2 gxy = ARmp8x8(LocalThreadId.x) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
	CurrFilter(gxy);
	gxy.x += 8u;
	CurrFilter(gxy);
	gxy.y += 8u;
	CurrFilter(gxy);
	gxy.x -= 8u;
	CurrFilter(gxy);
}
