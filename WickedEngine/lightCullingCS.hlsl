#include "globals.hlsli"
#include "cullingShaderHF.hlsli"

RWTEXTURE2D(DebugTexture, float4, 0);

STRUCTUREDBUFFER(in_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);

struct Light
{
	float3 PositionVS; // View Space!
	float Range;
	float4 col;
};
STRUCTUREDBUFFER(Lights, Light, SBSLOT_LIGHTARRAY);
#define lightCount ((uint)g_xColor.x)


groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum GroupFrustum;

groupshared uint visibleLightCount = 0;
groupshared uint visibleLightIndices[1024];

groupshared uint _counter = 0;

#define BLOCK_SIZE 16
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN)
{
	// Calculate min & max depth in threadgroup / tile.
	int2 texCoord = IN.dispatchThreadID.xy;
	float fDepth = texture_depth.Load(int3(texCoord, 0)).r;

	uint uDepth = asuint(fDepth);

	if (IN.groupIndex == 0) // Avoid contention by other threads in the group.
	{
		uMinDepth = 0xffffffff;
		uMaxDepth = 0;
		//o_LightCount = 0;
		//t_LightCount = 0;
		GroupFrustum = in_Frustums[IN.groupID.x + (IN.groupID.y * numThreadGroups.x)];
	}

	GroupMemoryBarrierWithGroupSync();

	InterlockedMin(uMinDepth, uDepth);
	InterlockedMax(uMaxDepth, uDepth);

	GroupMemoryBarrierWithGroupSync();

	float fMinDepth = asfloat(uMinDepth);
	float fMaxDepth = asfloat(uMaxDepth);

	// Convert depth values to view space.
	float minDepthVS = ScreenToView(float4(0, 0, fMinDepth, 1)).z;
	float maxDepthVS = ScreenToView(float4(0, 0, fMaxDepth, 1)).z;
	float nearClipVS = ScreenToView(float4(0, 0, 0, 1)).z;

	// Clipping plane for minimum depth value 
	// (used for testing lights within the bounds of opaque geometry).
	Plane minPlane = { float3(0, 0, -1), -minDepthVS };

	// Cull lights
	// Each thread in a group will cull 1 light until all lights have been culled.
	for (uint i = IN.groupIndex; i < lightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	{
		//if (Lights[i].Enabled)
		{
			Light light = Lights[i];

			//switch (light.Type)
			//{
			//case POINT_LIGHT:
			//{
				Sphere sphere = { light.PositionVS.xyz, light.Range };
				if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
				{
					//// Add light to light list for transparent geometry.
					//t_AppendLight(i);

					//if (!SphereInsidePlane(sphere, minPlane))
					//{
					//	// Add light to light list for opaque geometry.
					//	o_AppendLight(i);
					//}

					InterlockedAdd(_counter, 1);
				}
			//}
			//break;
			//case SPOT_LIGHT:
			//{
			//	float coneRadius = tan(radians(light.SpotlightAngle)) * light.Range;
			//	Cone cone = { light.PositionVS.xyz, light.Range, light.DirectionVS.xyz, coneRadius };
			//	if (ConeInsideFrustum(cone, GroupFrustum, nearClipVS, maxDepthVS))
			//	{
			//		// Add light to light list for transparent geometry.
			//		t_AppendLight(i);

			//		if (!ConeInsidePlane(cone, minPlane))
			//		{
			//			// Add light to light list for opaque geometry.
			//			o_AppendLight(i);
			//		}
			//	}
			//}
			//break;
			//case DIRECTIONAL_LIGHT:
			//{
			//	// Directional lights always get added to our light list.
			//	// (Hopefully there are not too many directional lights!)
			//	t_AppendLight(i);
			//	o_AppendLight(i);
			//}
			//break;
			//}
		}
	}

	// Wait till all threads in group have caught up.
	GroupMemoryBarrierWithGroupSync();

	//// Update global memory with visible light buffer.
	//// First update the light grid (only thread 0 in group needs to do this)
	//if (IN.groupIndex == 0)
	//{
	//	// Update light grid for opaque geometry.
	//	InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
	//	o_LightGrid[IN.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);

	//	// Update light grid for transparent geometry.
	//	InterlockedAdd(t_LightIndexCounter[0], t_LightCount, t_LightIndexStartOffset);
	//	t_LightGrid[IN.groupID.xy] = uint2(t_LightIndexStartOffset, t_LightCount);
	//}

	//GroupMemoryBarrierWithGroupSync();

	//// Now update the light index list (all threads).
	//// For opaque goemetry.
	//for (i = IN.groupIndex; i < o_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	//{
	//	o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
	//}
	//// For transparent geometry.
	//for (i = IN.groupIndex; i < t_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	//{
	//	t_LightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
	//}

	//// Update the debug texture output.
	//if (IN.groupThreadID.x == 0 || IN.groupThreadID.y == 0)
	//{
	//	DebugTexture[texCoord] = float4(0, 0, 0, 0.9f);
	//}
	//else if (IN.groupThreadID.x == 1 || IN.groupThreadID.y == 1)
	//{
	//	DebugTexture[texCoord] = float4(1, 1, 1, 0.5f);
	//}
	//else if (o_LightCount > 0)
	//{
	//	float normalizedLightCount = o_LightCount / 50.0f;
	//	float4 lightCountHeatMapColor = LightCountHeatMap.SampleLevel(LinearClampSampler, float2(normalizedLightCount, 0), 0);
	//	DebugTexture[texCoord] = lightCountHeatMapColor;
	//}
	//else
	//{
	//	DebugTexture[texCoord] = float4(0, 0, 0, 1);
	//}

	//DebugTexture[texCoord] = float4(GroupFrustum.planes[0].N,1);
	DebugTexture[texCoord] = float4((float)_counter/ (float)lightCount,0,0,1);
}