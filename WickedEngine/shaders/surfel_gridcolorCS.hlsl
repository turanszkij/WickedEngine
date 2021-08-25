#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelCellBuffer, uint, TEXSLOT_ONDEMAND2);

RWTEXTURE3D(surfelGridColorTexture, float3, 0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	SurfelGridCell cell = surfelGridBuffer[surfel_cellindex(DTid)];

	float4 color = 0;
	for (uint i = 0; i < cell.count; ++i)
	{
		uint surfel_index = surfelCellBuffer[cell.offset + i];
		Surfel surfel = surfelBuffer[surfel_index];
		color += float4(surfel.color, surfel.radius / SURFEL_MAX_RADIUS);
	}
	if (color.a > 0.01)
	{
		color /= color.a;
	}
	else
	{
		color = 0;
	}

	surfelGridColorTexture[DTid] = color.rgb;
}
