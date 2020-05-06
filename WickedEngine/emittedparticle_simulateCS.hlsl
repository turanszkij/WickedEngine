#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWRAWBUFFER(counterBuffer, 4);
RWSTRUCTUREDBUFFER(distanceBuffer, float, 6);

#define NUM_LDS_FORCEFIELDS 32
struct LDS_ForceField
{
	uint type;
	float3 position;
	float gravity;
	float range_rcp;
	float3 normal;
};
groupshared LDS_ForceField forceFields[NUM_LDS_FORCEFIELDS];

#define SPH_FLOOR_COLLISION
#define SPH_BOX_COLLISION


[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	//uint aliveCount = counterBuffer[0].aliveCount;
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	// Load the forcefields into LDS:
	uint numForceFields = min(g_xFrame_ForceFieldArrayCount, NUM_LDS_FORCEFIELDS);
	if (Gid < numForceFields)
	{
		uint forceFieldID = g_xFrame_ForceFieldArrayOffset + Gid;
		ShaderEntity forceField = EntityArray[forceFieldID];

		forceFields[Gid].type = (uint)forceField.GetType();
		forceFields[Gid].position = forceField.positionWS;
		forceFields[Gid].gravity = forceField.energy;
		forceFields[Gid].range_rcp = forceField.range; // it is actually uploaded from CPU as 1.0f / range
		forceFields[Gid].normal = forceField.directionWS;
	}

	GroupMemoryBarrierWithGroupSync();

	if (DTid.x < aliveCount)
	{
		// simulation can be either fixed or variable timestep:
		const float dt = xEmitterFixedTimestep >= 0 ? xEmitterFixedTimestep : g_xFrame_DeltaTime;

		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];

		if (particle.life > 0)
		{
			// simulate:

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

				particle.force += dir * forceField.gravity * (1 - saturate(dist * forceField.range_rcp));
			}


#ifdef DEPTHCOLLISIONS

			// NOTE: We are using the textures from previous frame, so reproject against those! (PrevVP)

			float4 pos2D = mul(g_xFrame_MainCamera_PrevVP, float4(particle.position, 1));
			pos2D.xyz /= pos2D.w;

			if (pos2D.x > -1 && pos2D.x < 1 && pos2D.y > -1 && pos2D.y < 1)
			{
				float2 uv = pos2D.xy * float2(0.5f, -0.5f) + 0.5f;
				uint2 pixel = uv * g_xFrame_InternalResolution;

				float depth0 = texture_depth[pixel];
				float surfaceLinearDepth = getLinearDepth(depth0);
				float surfaceThickness = 1.5f;

				float lifeLerp = 1 - particle.life / particle.maxLife;
				float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

				// check if particle is colliding with the depth buffer, but not completely behind it:
				if ((pos2D.w + particleSize > surfaceLinearDepth) && (pos2D.w - particleSize < surfaceLinearDepth + surfaceThickness))
				{
					// Calculate surface normal and bounce off the particle:
					float depth1 = texture_depth[pixel + uint2(1, 0)];
					float depth2 = texture_depth[pixel + uint2(0, -1)];

					float3 p0 = reconstructPosition(uv, depth0, g_xFrame_MainCamera_PrevInvVP);
					float3 p1 = reconstructPosition(uv + float2(1, 0) * g_xFrame_InternalResolution_rcp, depth1, g_xFrame_MainCamera_PrevInvVP);
					float3 p2 = reconstructPosition(uv + float2(0, -1) * g_xFrame_InternalResolution_rcp, depth2, g_xFrame_MainCamera_PrevInvVP);

					float3 surfaceNormal = normalize(cross(p2 - p0, p1 - p0));

					if (dot(particle.velocity, surfaceNormal) < 0)
					{
						const float restitution = 0.98f;
						particle.velocity = reflect(particle.velocity, surfaceNormal) * restitution;
					}
				}
			}

#endif // DEPTHCOLLISIONS

			// integrate:
			particle.velocity += particle.force * dt;
			particle.position += particle.velocity * dt;

			// reset force for next frame:
			particle.force = 0;

			[branch]
			if (xEmitterOptions & EMITTER_OPTION_BIT_SPH_ENABLED)
			{
				// drag: 
				particle.velocity *= 0.98f;

				// debug collisions:

				float elastic = 0.6;

				float lifeLerp = 1 - particle.life / particle.maxLife;
				float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

#ifdef SPH_FLOOR_COLLISION
				// floor collision:
				if (particle.position.y - particleSize < 0)
				{
					particle.position.y = particleSize;
					particle.velocity.y *= -elastic;
				}
#endif // FLOOR_COLLISION


#ifdef SPH_BOX_COLLISION
				// box collision:
				float3 extent = float3(40, 0, 22);
				if (particle.position.x + particleSize > extent.x)
				{
					particle.position.x = extent.x - particleSize;
					particle.velocity.x *= -elastic;
				}
				if (particle.position.x - particleSize < -extent.x)
				{
					particle.position.x = -extent.x + particleSize;
					particle.velocity.x *= -elastic;
				}
				if (particle.position.z + particleSize > extent.z)
				{
					particle.position.z = extent.z - particleSize;
					particle.velocity.z *= -elastic;
				}
				if (particle.position.z - particleSize < -extent.z)
				{
					particle.position.z = -extent.z + particleSize;
					particle.velocity.z *= -elastic;
				}
#endif // BOX_COLLISION

			}

			particle.life -= dt;

			// write back simulated particle:
			particleBuffer[particleIndex] = particle;

			// add to new alive list:
			uint newAliveIndex;
			counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, 1, newAliveIndex);
			aliveBuffer_NEW[newAliveIndex] = particleIndex;

#ifdef SORTING
			// store squared distance to main camera:
			float3 eyeVector = particle.position - g_xCamera_CamPos;
			float distSQ = dot(eyeVector, eyeVector);
			distanceBuffer[particleIndex] = -distSQ; // this can be negated to modify sorting order here instead of rewriting sorting shaders...
#endif // SORTING

		}
		else
		{
			// kill:
			uint deadIndex;
			counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_DEADCOUNT, 1, deadIndex);
			deadBuffer[deadIndex] = particleIndex;
		}
	}

}
