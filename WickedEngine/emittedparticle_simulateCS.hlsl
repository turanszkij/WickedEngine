#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "reconstructPositionHF.hlsli"
#include "depthConvertHF.hlsli"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);
RWRAWBUFFER(indirectBuffers, 5);
RWSTRUCTUREDBUFFER(distanceBuffer, float, 6);

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


[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
#ifndef SHADERCOMPILER_SPIRV
	// TODO: this shader crashes the Vulkan compute pipeline creation...

	uint aliveCount = counterBuffer[0].aliveCount;

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

	if (DTid.x < aliveCount)
	{
		const float dt = g_xFrame_DeltaTime;

		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		if (particle.life > 0)
		{
			// simulate:

			float3 force = 0;
			for (uint i = 0; i < numForceFields; ++i)
			{
				LDS_ForceField forceField = forceFields[i];

				float3 dir = forceField.position - particle.position;
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
			particle.velocity += force * dt;


#ifdef DEPTHCOLLISIONS

			// NOTE: We are using the textures from previous frame, so reproject against those! (PrevVP)

			float4 pos2D = mul(float4(particle.position, 1), g_xFrame_MainCamera_PrevVP);
			pos2D.xyz /= pos2D.w;

			if (pos2D.x > -1 && pos2D.x < 1 && pos2D.y > -1 && pos2D.y < 1)
			{
				float2 uv = pos2D.xy * float2(0.5f, -0.5f) + 0.5f;
				uint2 pixel = uv * g_xWorld_InternalResolution;

				float depth0 = texture_depth[pixel + uint2(0, 0)];
				float surfaceLinearDepth = getLinearDepth(depth0);
				float surfaceThickness = 1.5f;

				float lifeLerp = 1 - particle.life / particle.maxLife;
				float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

				// check if particle is colliding with the depth buffer, but not completely behind it:
				if ((pos2D.w + particleSize > surfaceLinearDepth) && (pos2D.w - particleSize < surfaceLinearDepth + surfaceThickness))
				{
					// Calculate surface normal and bounce off the particle:
					float depth1 = texture_depth[pixel + uint2(1, 0)];
					float depth2 = texture_depth[pixel + uint2(0, 1)];

					float3 p0 = getPositionEx(uv, depth0, g_xFrame_MainCamera_PrevInvVP);
					float3 p1 = getPositionEx(uv, depth1, g_xFrame_MainCamera_PrevInvVP);
					float3 p2 = getPositionEx(uv, depth2, g_xFrame_MainCamera_PrevInvVP);

					float3 surfaceNormal = normalize(cross(p2 - p0, p1 - p0));

					const float restitution = 0.98f;
					particle.velocity = reflect(particle.velocity, surfaceNormal) * restitution;
				}
			}

#endif // DEPTHCOLLISIONS

			particle.position += particle.velocity * dt;

			particle.life -= dt;

			// write back simulated particle:
			particleBuffer[particleIndex] = particle;

			// add to new alive list:
			uint newAliveIndex;
			indirectBuffers.InterlockedAdd(ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, 6, newAliveIndex); // write the draw argument buffer, which should contain particle count * 6
			newAliveIndex /= 6; // draw arg buffer contains particle count * 6, so just divide to retrieve correct index
			aliveBuffer_NEW[newAliveIndex] = particleIndex;

#ifdef SORTING
			// store squared distance to main camera:
			float3 eyeVector = particle.position - g_xFrame_MainCamera_CamPos;
			float distSQ = dot(eyeVector, eyeVector);
			distanceBuffer[particleIndex] = -distSQ; // this can be negated to modify sorting order here instead of rewriting sorting shaders...
#endif // SORTING

		}
		else
		{
			// kill:
			uint deadIndex;
			InterlockedAdd(counterBuffer[0].deadCount, 1, deadIndex);
			deadBuffer[deadIndex] = particleIndex;
		}
	}

#endif // SHADERCOMPILER_SPIRV
}
