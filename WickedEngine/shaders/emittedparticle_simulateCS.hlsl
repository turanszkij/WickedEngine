#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWRAWBUFFER(counterBuffer, 4);
RWSTRUCTUREDBUFFER(distanceBuffer, float, 6);
RWRAWBUFFER(vertexBuffer_POS, 7);
RWRAWBUFFER(vertexBuffer_TEX, 8);
RWRAWBUFFER(vertexBuffer_TEX2, 9);
RWRAWBUFFER(vertexBuffer_COL, 10);
RWSTRUCTUREDBUFFER(culledIndirectionBuffer, uint, 11);
RWSTRUCTUREDBUFFER(culledIndirectionBuffer2, uint, 12);

#define SPH_FLOOR_COLLISION
#define SPH_BOX_COLLISION


[numthreads(THREADCOUNT_SIMULATION, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint Gid : SV_GroupIndex)
{
	//uint aliveCount = counterBuffer[0].aliveCount;
	uint aliveCount = counterBuffer.Load(PARTICLECOUNTER_OFFSET_ALIVECOUNT);

	if (DTid.x < aliveCount)
	{
		// simulation can be either fixed or variable timestep:
		const float dt = xEmitterFixedTimestep >= 0 ? xEmitterFixedTimestep : g_xFrame.DeltaTime;

		uint particleIndex = aliveBuffer_CURRENT[DTid.x];
		Particle particle = particleBuffer[particleIndex];
		uint v0 = particleIndex * 4;

		if (particle.life > 0)
		{
			// simulate:
			for (uint i = 0; i < g_xFrame.ForceFieldArrayCount; ++i)
			{
				ShaderEntity forceField = load_entity(g_xFrame.ForceFieldArrayOffset + i);

				[branch]
				if (forceField.layerMask & xEmitterLayerMask)
				{
					float3 dir = forceField.position - particle.position;
					float dist;
					if (forceField.GetType() == ENTITY_TYPE_FORCEFIELD_POINT) // point-based force field
					{
						dist = length(dir);
					}
					else // planar force field
					{
						dist = dot(forceField.GetDirection(), dir);
						dir = forceField.GetDirection();
					}

					particle.force += dir * forceField.GetEnergy() * (1 - saturate(dist * forceField.GetRange())); // GetRange() is actually uploaded as 1.0 / range
				}
			}


#ifdef DEPTHCOLLISIONS
			// NOTE: We are using the textures from previous frame, so reproject against those! (PrevVP)

			float4 pos2D = mul(GetCamera().PrevVP, float4(particle.position, 1));
			pos2D.xyz /= pos2D.w;

			if (pos2D.x > -1 && pos2D.x < 1 && pos2D.y > -1 && pos2D.y < 1)
			{
				float2 uv = pos2D.xy * float2(0.5f, -0.5f) + 0.5f;
				uint2 pixel = uv * GetCamera().InternalResolution;

				float depth0 = texture_depth_history[pixel];
				float surfaceLinearDepth = getLinearDepth(depth0);
				float surfaceThickness = 1.5f;

				float lifeLerp = 1 - particle.life / particle.maxLife;
				float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

				// check if particle is colliding with the depth buffer, but not completely behind it:
				if ((pos2D.w + particleSize > surfaceLinearDepth) && (pos2D.w - particleSize < surfaceLinearDepth + surfaceThickness))
				{
					// Calculate surface normal and bounce off the particle:
					float depth1 = texture_depth_history[pixel + uint2(1, 0)];
					float depth2 = texture_depth_history[pixel + uint2(0, -1)];

					float3 p0 = reconstructPosition(uv, depth0, GetCamera().PrevInvVP);
					float3 p1 = reconstructPosition(uv + float2(1, 0) * GetCamera().InternalResolution_rcp, depth1, GetCamera().PrevInvVP);
					float3 p2 = reconstructPosition(uv + float2(0, -1) * GetCamera().InternalResolution_rcp, depth2, GetCamera().PrevInvVP);

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
			particle.force += xParticleGravity;
			particle.velocity += particle.force * dt;
			particle.position += particle.velocity * dt;

			// reset force for next frame:
			particle.force = 0;

			// drag: 
			particle.velocity *= xParticleDrag;

			float lifeLerp = 1 - particle.life / particle.maxLife;
			float particleSize = lerp(particle.sizeBeginEnd.x, particle.sizeBeginEnd.y, lifeLerp);

			[branch]
			if (xEmitterOptions & EMITTER_OPTION_BIT_SPH_ENABLED)
			{
				// debug collisions:

				float elastic = 0.6;

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

			// Write out render buffers:
			//	These must be persistent, not culled (raytracing, surfels...)

			float opacity = saturate(lerp(1, 0, lifeLerp) * EmitterGetMaterial().baseColor.a);
			uint particleColorPacked = (particle.color_mirror & 0x00FFFFFF) | (uint(opacity * 255.0f) << 24u);

			float rotation = lifeLerp * particle.rotationalVelocity;
			float2x2 rot = float2x2(
				cos(rotation), -sin(rotation),
				sin(rotation), cos(rotation)
				);

			// Sprite sheet frame:
			const float spriteframe = xEmitterFrameRate == 0 ?
				lerp(xEmitterFrameStart, xEmitterFrameCount, lifeLerp) :
				((xEmitterFrameStart + particle.life * xEmitterFrameRate) % xEmitterFrameCount);
			const uint currentFrame = floor(spriteframe);
			const uint nextFrame = ceil(spriteframe);
			const float frameBlend = frac(spriteframe);
			uint2 offset = uint2(currentFrame % xEmitterFramesXY.x, currentFrame / xEmitterFramesXY.x);
			uint2 offset2 = uint2(nextFrame % xEmitterFramesXY.x, nextFrame / xEmitterFramesXY.x);

			for (uint vertexID = 0; vertexID < 4; ++vertexID)
			{
				// expand the point into a billboard in view space:
				float3 quadPos = BILLBOARD[vertexID];
				quadPos.x = particle.color_mirror & 0x10000000 ? -quadPos.x : quadPos.x;
				quadPos.y = particle.color_mirror & 0x20000000 ? -quadPos.y : quadPos.y;
				float2 uv = quadPos.xy * float2(0.5f, -0.5f) + 0.5f;
				float2 uv2 = uv;

				// sprite sheet UV transform:
				uv.xy += offset;
				uv.xy *= xEmitterTexMul;
				uv2.xy += offset2;
				uv2.xy *= xEmitterTexMul;

				// rotate the billboard:
				quadPos.xy = mul(quadPos.xy, rot);

				// scale the billboard:
				quadPos *= particleSize;

				// scale the billboard along view space motion vector:
				float3 velocity = mul((float3x3)GetCamera().View, particle.velocity);
				quadPos += dot(quadPos, velocity) * velocity * xParticleMotionBlurAmount;

				// rotate the billboard to face the camera:
				quadPos = mul(quadPos, (float3x3)GetCamera().View); // reversed mul for inverse camera rotation!

				// write out vertex:
				uint4 data;
				data.xyz = asuint(particle.position + quadPos);
				data.w = pack_unitvector(normalize(-GetCamera().At));
				vertexBuffer_POS.Store4((v0 + vertexID) * 16, data);
				vertexBuffer_TEX.Store((v0 + vertexID) * 4, pack_half2(uv));
				vertexBuffer_TEX2.Store((v0 + vertexID) * 4, pack_half2(uv2));
				vertexBuffer_COL.Store((v0 + vertexID) * 4, particleColorPacked);
			}

			// Frustum culling:
			uint infrustum = 1;
			float3 center = particle.position;
			float radius = -particleSize;
			infrustum &= dot(GetCamera().FrustumPlanes[0], float4(center, 1)) > radius;
			infrustum &= dot(GetCamera().FrustumPlanes[1], float4(center, 1)) > radius;
			infrustum &= dot(GetCamera().FrustumPlanes[2], float4(center, 1)) > radius;
			infrustum &= dot(GetCamera().FrustumPlanes[3], float4(center, 1)) > radius;
			infrustum &= dot(GetCamera().FrustumPlanes[4], float4(center, 1)) > radius;
			infrustum &= dot(GetCamera().FrustumPlanes[5], float4(center, 1)) > radius;

			if (infrustum)
			{
				uint prevCount;
				counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_CULLEDCOUNT, 1, prevCount);

				culledIndirectionBuffer[prevCount] = prevCount;
				culledIndirectionBuffer2[prevCount] = particleIndex;

#ifdef SORTING
				// store squared distance to main camera:
				float3 eyeVector = particle.position - GetCamera().CamPos;
				float distSQ = dot(eyeVector, eyeVector);
				distanceBuffer[prevCount] = -distSQ; // this can be negated to modify sorting order here instead of rewriting sorting shaders...
#endif // SORTING
			}

		}
		else
		{
			// kill:
			uint deadIndex;
			counterBuffer.InterlockedAdd(PARTICLECOUNTER_OFFSET_DEADCOUNT, 1, deadIndex);
			deadBuffer[deadIndex] = particleIndex;

			vertexBuffer_POS.Store4((v0 + 0) * 16, 0);
			vertexBuffer_POS.Store4((v0 + 1) * 16, 0);
			vertexBuffer_POS.Store4((v0 + 2) * 16, 0);
			vertexBuffer_POS.Store4((v0 + 3) * 16, 0);
		}
	}

}
