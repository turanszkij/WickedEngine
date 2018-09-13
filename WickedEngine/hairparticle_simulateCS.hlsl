#include "globals.hlsli"
#include "ShaderInterop_HairParticle.h"

RWRAWBUFFER(particleBuffer, 0);

RAWBUFFER(targetBuffer, 0);

#define NUM_LDS_FORCEFIELDS 32
struct LDS_ForceField
{
	uint type;
	float3 position;
	float gravity;
	float range_inverse;
	float3 normal;
};
groupshared LDS_ForceField forceFields[NUM_LDS_FORCEFIELDS];

[numthreads(THREADCOUNT_SIMULATEHAIR, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	// Load the forcefields into LDS:
	uint numForceFields = min(g_xFrame_ForceFieldArrayCount, NUM_LDS_FORCEFIELDS);
	if (Gid < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldArrayOffset + Gid;
		ShaderEntityType forceField = EntityArray[forceFieldID];

		forceFields[Gid].type = (uint)forceField.type;
		forceFields[Gid].position = forceField.positionWS;
		forceFields[Gid].gravity = forceField.energy;
		forceFields[Gid].range_inverse = forceField.range;
		forceFields[Gid].normal = forceField.directionWS;
	}

	GroupMemoryBarrierWithGroupSync();


	const uint instanceID = DTid.x;

	// Fetch from particle buffer:
	const uint fetchAddress = instanceID * particleBuffer_stride;
	float4 posLen = asfloat(particleBuffer.Load4(fetchAddress));
	uint normalRand = particleBuffer.Load(fetchAddress + 16);

	// Decompress particle properties:
	float3 pos = posLen.xyz;
	float len = posLen.w * xLength;
	float3 normal;
	normal.x = (normalRand >> 0) & 0x000000FF;
	normal.y = (normalRand >> 8) & 0x000000FF;
	normal.z = (normalRand >> 16) & 0x000000FF;
	normal = normal / 255.0f * 2 - 1;

	// Fetch from target buffer
	uint uTarget = targetBuffer.Load(instanceID * 4);

	// Decompress target:
	float3 target;
	target.x = (uTarget >> 0) & 0x000000FF;
	target.y = (uTarget >> 8) & 0x000000FF;
	target.z = (uTarget >> 16) & 0x000000FF;
	target = target / 255.0f * 2 - 1;

	// transform particle by the emitter object matrix:
	pos.xyz = mul(xWorld, float4(pos.xyz, 1)).xyz;
	normal = mul((float3x3)xWorld, normal);
	target = mul((float3x3)xWorld, target);

	// Accumulate forces:
	float3 force = 0;
	for (uint i = 0; i < numForceFields; ++i)
	{
		LDS_ForceField forceField = forceFields[i];

		float3 dir = forceField.position - pos;
		float dist;
		if (forceField.type == ENTITY_TYPE_FORCEFIELD_POINT) // point-based force field
		{
			dist = length(dir);
		}
		else // planar force field
		{
			dist = dot(forceField.normal, dir);
			dir = forceField.normal;
		}

		force += dir * forceField.gravity * (1 - saturate(dist * forceField.range_inverse));
	}

	// Apply forces:
	normal += force * g_xFrame_DeltaTime;

	// Apply rest:
	normal = lerp(normal, target, 0.1f);

	// Transform back to mesh space and renormalize:
	normal = mul(normal, (float3x3)xWorld); // transposed mul!
	normal = normalize(normal);


	// Compress normal and store:
	uint uNormal = 0;
	uNormal |= (uint)((normal.x * 0.5f + 0.5f) * 255.0f) << 0;
	uNormal |= (uint)((normal.y * 0.5f + 0.5f) * 255.0f) << 8;
	uNormal |= (uint)((normal.z * 0.5f + 0.5f) * 255.0f) << 16;
	uNormal |= normalRand & 0xFF000000;
	particleBuffer.Store(fetchAddress + 16, uNormal);
}
