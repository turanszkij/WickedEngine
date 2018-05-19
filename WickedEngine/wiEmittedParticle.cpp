#include "wiEmittedParticle.h"
#include "wiMath.h"
#include "wiLoader.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "wiTextureHelper.h"
#include "wiGPUSortLib.h"
#include "wiProfiler.h"

using namespace std;
using namespace wiGraphicsTypes;

VertexShader  *wiEmittedParticle::vertexShader = nullptr;
PixelShader   *wiEmittedParticle::pixelShader[PARTICLESHADERTYPE_COUNT] = {};
ComputeShader   *wiEmittedParticle::kickoffUpdateCS = nullptr, *wiEmittedParticle::finishUpdateCS = nullptr, *wiEmittedParticle::emitCS = nullptr, *wiEmittedParticle::sphpartitionCS = nullptr, 
				*wiEmittedParticle::sphpartitionoffsetsCS = nullptr, *wiEmittedParticle::sphpartitionoffsetsresetCS = nullptr, *wiEmittedParticle::sphdensityCS = nullptr, *wiEmittedParticle::sphforceCS = nullptr, 
				*wiEmittedParticle::simulateCS = nullptr, *wiEmittedParticle::simulateCS_SORTING = nullptr, *wiEmittedParticle::simulateCS_DEPTHCOLLISIONS = nullptr, *wiEmittedParticle::simulateCS_SORTING_DEPTHCOLLISIONS = nullptr;

BlendState		wiEmittedParticle::blendStates[BLENDMODE_COUNT];
RasterizerState		wiEmittedParticle::rasterizerState, wiEmittedParticle::wireFrameRS;
DepthStencilState	wiEmittedParticle::depthStencilState;
GraphicsPSO wiEmittedParticle::PSO[BLENDMODE_COUNT][PARTICLESHADERTYPE_COUNT];
GraphicsPSO wiEmittedParticle::PSO_wire;
ComputePSO wiEmittedParticle::CPSO_kickoffUpdate, wiEmittedParticle::CPSO_finishUpdate, wiEmittedParticle::CPSO_emit, wiEmittedParticle::CPSO_sphpartition, wiEmittedParticle::CPSO_sphpartitionoffsets, 
				wiEmittedParticle::CPSO_sphpartitionoffsetsreset, wiEmittedParticle::CPSO_sphdensity, wiEmittedParticle::CPSO_sphforce, wiEmittedParticle::CPSO_simulate,
				wiEmittedParticle::CPSO_simulate_SORTING, wiEmittedParticle::CPSO_simulate_DEPTHCOLLISIONS, wiEmittedParticle::CPSO_simulate_SORTING_DEPTHCOLLISIONS;

wiEmittedParticle::wiEmittedParticle()
{
	name = "";
	object = nullptr;
	materialName = "";
	material = nullptr;

	size = 1;
	random_factor = 0;
	normal_factor = 1;

	count = 1;
	life = 60;
	random_life = 0;
	emit = 0;

	scaleX = 1;
	scaleY = 1;
	rotation = 0;

	motionBlurAmount = 0.0f;

	SetMaxParticleCount(10000);
}
wiEmittedParticle::wiEmittedParticle(const std::string& newName, const std::string& newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot)
{
	name=newName;
	object=newObject;
	materialName = newMat;
	for (MeshSubset& subset : object->mesh->subsets)
	{
		if (!newMat.compare(subset.material->name)) {
			material = subset.material;
			break;
		}
	}

	size=newSize;
	random_factor=newRandomFac;
	normal_factor=newNormalFac;

	count=newCount;
	life=newLife;
	random_life=newRandLife;
	emit=0;
	
	scaleX=newScaleX;
	scaleY=newScaleY;
	rotation = newRot;

	motionBlurAmount = 0.0f;

	SetMaxParticleCount(10000);
}
wiEmittedParticle::wiEmittedParticle(const wiEmittedParticle& other)
{
	name = other.name + "0";
	object = other.object;
	materialName = other.materialName;
	material = other.material;
	size = other.size;
	random_factor = other.random_factor;
	normal_factor = other.normal_factor;
	count = other.count;
	life = other.life;
	random_life = other.random_life;
	emit = 0;
	scaleX = other.scaleX;
	scaleY = other.scaleY;
	rotation = other.rotation;
	motionBlurAmount = other.motionBlurAmount;

	SetMaxParticleCount(other.GetMaxParticleCount());
}

void wiEmittedParticle::SetMaxParticleCount(uint32_t value)
{
	buffersUpToDate = false;
	MAX_PARTICLES = value;
}

void wiEmittedParticle::CreateSelfBuffers()
{
	if (buffersUpToDate)
	{
		return;
	}
	buffersUpToDate = true;

	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(aliveList[0]);
	SAFE_DELETE(aliveList[1]);
	SAFE_DELETE(deadList);
	SAFE_DELETE(distanceBuffer);
	SAFE_DELETE(sphPartitionCellIndices);
	SAFE_DELETE(sphPartitionCellOffsets);
	SAFE_DELETE(densityBuffer);
	SAFE_DELETE(counterBuffer);
	SAFE_DELETE(indirectBuffers);
	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(debugDataReadbackBuffer);
	SAFE_DELETE(debugDataReadbackIndexBuffer);
	SAFE_DELETE(debugDataReadbackDistanceBuffer);

	particleBuffer = new GPUBuffer;
	aliveList[0] = new GPUBuffer;
	aliveList[1] = new GPUBuffer;
	deadList = new GPUBuffer;
	distanceBuffer = new GPUBuffer;
	sphPartitionCellIndices = new GPUBuffer;
	sphPartitionCellOffsets = new GPUBuffer;
	densityBuffer = new GPUBuffer;
	counterBuffer = new GPUBuffer;
	indirectBuffers = new GPUBuffer;
	constantBuffer = new GPUBuffer;
	debugDataReadbackBuffer = new GPUBuffer;
	debugDataReadbackIndexBuffer = new GPUBuffer;
	debugDataReadbackDistanceBuffer = new GPUBuffer;


	// GPU-local buffer descriptors:
	GPUBufferDesc bd;
	bd.Usage = USAGE_DEFAULT;
	bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	SubresourceData data;

	// Particle buffer:
	bd.StructureByteStride = sizeof(Particle);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, particleBuffer);

	// Alive index lists (double buffered):
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[0]);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, aliveList[1]);

	// Dead index list:
	uint32_t* indices = new uint32_t[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		indices[i] = i;
	}
	data.pSysMem = indices;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, deadList);
	SAFE_DELETE_ARRAY(indices);
	data.pSysMem = nullptr;

	// Distance buffer:
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	float* distances = new float[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		distances[i] = 0;
	}
	data.pSysMem = distances;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, distanceBuffer);
	SAFE_DELETE_ARRAY(distances);
	data.pSysMem = nullptr;

	// SPH Partitioning grid indices per particle:
	bd.StructureByteStride = sizeof(float); // really, it is uint, but sorting is performing comparisons on floats, so whateva
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, sphPartitionCellIndices);

	// SPH Partitioning grid cell offsets into particle index list:
	bd.StructureByteStride = sizeof(uint32_t);
	bd.ByteWidth = bd.StructureByteStride * SPH_PARTITION_BUCKET_COUNT;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, sphPartitionCellOffsets);

	// Density buffer (for SPH simulation):
	bd.StructureByteStride = sizeof(float);
	bd.ByteWidth = bd.StructureByteStride * MAX_PARTICLES;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, densityBuffer);

	// Particle System statistics:
	ParticleCounters counters;
	counters.aliveCount = 0;
	counters.deadCount = MAX_PARTICLES;
	counters.realEmitCount = 0;
	counters.aliveCount_afterSimulation = 0;

	data.pSysMem = &counters;
	bd.ByteWidth = sizeof(counters);
	bd.StructureByteStride = sizeof(counters);
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, counterBuffer);
	data.pSysMem = nullptr;

	// Indirect Execution buffer:
	bd.BindFlags = BIND_UNORDERED_ACCESS;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bd.ByteWidth = 
		sizeof(wiGraphicsTypes::IndirectDispatchArgs) + 
		sizeof(wiGraphicsTypes::IndirectDispatchArgs) + 
		sizeof(wiGraphicsTypes::IndirectDrawArgsInstanced);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, indirectBuffers);

	// Constant buffer:
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(EmittedParticleCB);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, constantBuffer);

	// Debug information CPU-readback buffer:
	{
		GPUBufferDesc debugBufDesc = counterBuffer->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackBuffer);
	}

	// Sorting debug buffers:
	{
		GPUBufferDesc debugBufDesc = aliveList[0]->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackIndexBuffer);
	}
	{
		GPUBufferDesc debugBufDesc = distanceBuffer->GetDesc();
		debugBufDesc.Usage = USAGE_STAGING;
		debugBufDesc.CPUAccessFlags = CPU_ACCESS_READ;
		debugBufDesc.BindFlags = 0;
		debugBufDesc.MiscFlags = 0;
		wiRenderer::GetDevice()->CreateBuffer(&debugBufDesc, nullptr, debugDataReadbackDistanceBuffer);
	}
}

uint32_t wiEmittedParticle::GetMemorySizeInBytes() const
{
	if (particleBuffer == nullptr)
		return 0;

	uint32_t retVal = 0;

	retVal += particleBuffer->GetDesc().ByteWidth;
	retVal += aliveList[0]->GetDesc().ByteWidth;
	retVal += aliveList[1]->GetDesc().ByteWidth;
	retVal += deadList->GetDesc().ByteWidth;
	retVal += distanceBuffer->GetDesc().ByteWidth;
	retVal += sphPartitionCellIndices->GetDesc().ByteWidth;
	retVal += sphPartitionCellOffsets->GetDesc().ByteWidth;
	retVal += densityBuffer->GetDesc().ByteWidth;
	retVal += counterBuffer->GetDesc().ByteWidth;
	retVal += indirectBuffers->GetDesc().ByteWidth;
	retVal += constantBuffer->GetDesc().ByteWidth;

	return retVal;
}

XMFLOAT3 wiEmittedParticle::GetPosition() const
{
	return object == nullptr ? XMFLOAT3(0, 0, 0) : object->translation;
}

void wiEmittedParticle::Update(float dt)
{
	if (PAUSED)
		return;

	emit += (float)count*dt;
}
void wiEmittedParticle::Burst(float num)
{
	if (PAUSED)
		return;

	emit += num;
}
void wiEmittedParticle::Restart()
{
	buffersUpToDate = false;
	PAUSED = false;
}

//#define DEBUG_SORTING // slow but great for debug!!
void wiEmittedParticle::UpdateRenderData(GRAPHICSTHREAD threadID)
{
	CreateSelfBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();


	if (!PAUSED)
	{

		device->EventBegin("UpdateEmittedParticles", threadID);

		EmittedParticleCB cb;
		cb.xEmitterWorld = object->world;
		cb.xEmitCount = (UINT)emit;
		cb.xEmitterMeshIndexCount = (UINT)object->mesh->indices.size();
		cb.xEmitterMeshVertexPositionStride = sizeof(Mesh::Vertex_POS);
		cb.xEmitterRandomness = wiRandom::getRandom(0, 1000) * 0.001f;
		cb.xParticleLifeSpan = life / 60.0f;
		cb.xParticleLifeSpanRandomness = random_life;
		cb.xParticleNormalFactor = normal_factor;
		cb.xParticleRandomFactor = random_factor;
		cb.xParticleScaling = scaleX;
		cb.xParticleSize = size;
		cb.xParticleMotionBlurAmount = motionBlurAmount;
		cb.xParticleRotation = rotation * XM_PI * 60;
		cb.xParticleColor = wiMath::CompressColor(XMFLOAT4(material->baseColor.x, material->baseColor.y, material->baseColor.z, 1));
		cb.xEmitterOpacity = material->alpha;
		cb.xParticleMass = mass;
		cb.xEmitterMaxParticleCount = MAX_PARTICLES;
		cb.xEmitterFixedTimestep = FIXED_TIMESTEP;

		// SPH:
		cb.xSPH_h = SPH_h;
		cb.xSPH_h_rcp = 1.0f / SPH_h;
		cb.xSPH_h2 = SPH_h * SPH_h;
		cb.xSPH_h3 = cb.xSPH_h2 * SPH_h;
		const float h6 = cb.xSPH_h2 * cb.xSPH_h2 * cb.xSPH_h2;
		const float h9 = cb.xSPH_h3 * cb.xSPH_h3;
		cb.xSPH_poly6_constant = (315.0f / (64.0f * XM_PI * h9));
		cb.xSPH_spiky_constant = (-45 / (XM_PI * h6));
		cb.xSPH_K = SPH_K;
		cb.xSPH_p0 = SPH_p0;
		cb.xSPH_e = SPH_e;
		cb.xSPH_ENABLED = SPH_FLUIDSIMULATION ? 1 : 0;

		device->UpdateBuffer(constantBuffer, &cb, threadID);
		device->BindConstantBuffer(CS, constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), threadID);

		GPUResource* uavs[] = {
			particleBuffer,
			aliveList[0], // CURRENT alivelist
			aliveList[1], // NEW alivelist
			deadList,
			counterBuffer,
			indirectBuffers,
			distanceBuffer,
		};
		device->BindUnorderedAccessResourcesCS(uavs, 0, ARRAYSIZE(uavs), threadID);

		GPUResource* resources[] = {
			wiTextureHelper::getInstance()->getRandom64x64(),
			object->mesh->indexBuffer,
			object->mesh->vertexBuffer_POS,
		};
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		GPUResource* indres[] = {
			indirectBuffers
		};
		device->TransitionBarrier(indres, 1, RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_UNORDERED_ACCESS, threadID);

		// kick off updating, set up state
		device->EventBegin("KickOff Update", threadID);
		device->BindComputePSO(&CPSO_kickoffUpdate, threadID);
		device->Dispatch(1, 1, 1, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);

		device->TransitionBarrier(indres, 1, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT, threadID);

		// emit the required amount if there are free slots in dead list
		device->EventBegin("Emit", threadID);
		device->BindComputePSO(&CPSO_emit, threadID);
		device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHEMIT, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);

		if (SPH_FLUIDSIMULATION)
		{
			wiProfiler::GetInstance().BeginRange("SPH - Simulation", wiProfiler::DOMAIN_GPU, threadID);

			// Smooth Particle Hydrodynamics:
			device->EventBegin("SPH - Simulation", threadID);

#ifdef SPH_USE_ACCELERATION_GRID
			// 1.) Assign particles into partitioning grid:
			device->EventBegin("Partitioning", threadID);
			device->BindComputePSO(&CPSO_sphpartition, threadID);
			device->UnBindUnorderedAccessResources(0, 8, threadID);
			GPUResource* res_partition[] = {
				aliveList[0], // CURRENT alivelist
				counterBuffer,
				particleBuffer,
			};
			device->BindResources(CS, res_partition, 0, ARRAYSIZE(res_partition), threadID);
			GPUResource* uav_partition[] = {
				sphPartitionCellIndices,
			};
			device->BindUnorderedAccessResourcesCS(uav_partition, 0, ARRAYSIZE(uav_partition), threadID);
			device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_partition, ARRAYSIZE(uav_partition), threadID);
			device->EventEnd(threadID);

			// 2.) Sort particle index list based on partition grid cell index:
			wiGPUSortLib::Sort(MAX_PARTICLES, sphPartitionCellIndices, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT, aliveList[0], threadID);

			// 3.) Reset grid cell offset buffer with invalid offsets (max uint):
			device->EventBegin("PartitionOffsetsReset", threadID);
			device->BindComputePSO(&CPSO_sphpartitionoffsetsreset, threadID);
			device->UnBindUnorderedAccessResources(0, 8, threadID);
			GPUResource* uav_partitionoffsets[] = {
				sphPartitionCellOffsets,
			};
			device->BindUnorderedAccessResourcesCS(uav_partitionoffsets, 0, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->Dispatch((UINT)ceilf((float)SPH_PARTITION_BUCKET_COUNT / (float)THREADCOUNT_SIMULATION), 1, 1, threadID);
			device->UAVBarrier(uav_partitionoffsets, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->EventEnd(threadID);

			// 4.) Assemble grid cell offsets from the sorted particle index list <--> grid cell index list connection:
			device->EventBegin("PartitionOffsets", threadID);
			device->BindComputePSO(&CPSO_sphpartitionoffsets, threadID);
			GPUResource* res_partitionoffsets[] = {
				aliveList[0], // CURRENT alivelist
				counterBuffer,
				sphPartitionCellIndices,
			};
			device->BindResources(CS, res_partitionoffsets, 0, ARRAYSIZE(res_partitionoffsets), threadID);
			device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_partitionoffsets, ARRAYSIZE(uav_partitionoffsets), threadID);
			device->EventEnd(threadID);

#endif // SPH_USE_ACCELERATION_GRID

			// 5.) Compute particle density field:
			device->EventBegin("Density Evaluation", threadID);
			device->BindComputePSO(&CPSO_sphdensity, threadID);
			device->UnBindUnorderedAccessResources(0, 8, threadID);
			GPUResource* res_density[] = {
				aliveList[0], // CURRENT alivelist
				counterBuffer,
				particleBuffer,
				sphPartitionCellIndices,
				sphPartitionCellOffsets,
			};
			device->BindResources(CS, res_density, 0, ARRAYSIZE(res_density), threadID);
			GPUResource* uav_density[] = {
				densityBuffer
			};
			device->BindUnorderedAccessResourcesCS(uav_density, 0, ARRAYSIZE(uav_density), threadID);
			device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_density, ARRAYSIZE(uav_density), threadID);
			device->EventEnd(threadID);

			// 6.) Compute particle pressure forces:
			device->EventBegin("Force Evaluation", threadID);
			device->BindComputePSO(&CPSO_sphforce, threadID);
			device->UnBindUnorderedAccessResources(0, 8, threadID);
			GPUResource* res_force[] = {
				aliveList[0], // CURRENT alivelist
				counterBuffer,
				densityBuffer,
				sphPartitionCellIndices,
				sphPartitionCellOffsets,
			};
			device->BindResources(CS, res_force, 0, ARRAYSIZE(res_force), threadID);
			GPUResource* uav_force[] = {
				particleBuffer,
			};
			device->BindUnorderedAccessResourcesCS(uav_force, 0, ARRAYSIZE(uav_force), threadID);
			device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
			device->UAVBarrier(uav_force, ARRAYSIZE(uav_force), threadID);
			device->EventEnd(threadID);

			device->UnBindResources(0, 3, threadID);
			device->UnBindUnorderedAccessResources(0, 8, threadID);

			device->EventEnd(threadID);

			wiProfiler::GetInstance().EndRange(threadID);
		}

		device->EventBegin("Simulate", threadID);
		device->BindUnorderedAccessResourcesCS(uavs, 0, ARRAYSIZE(uavs), threadID);
		device->BindResources(CS, resources, TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		// update CURRENT alive list, write NEW alive list
		if (SORTING)
		{
			if (DEPTHCOLLISIONS)
			{
				device->BindComputePSO(&CPSO_simulate_SORTING_DEPTHCOLLISIONS, threadID);
			}
			else
			{
				device->BindComputePSO(&CPSO_simulate_SORTING, threadID);
			}
		}
		else
		{
			if (DEPTHCOLLISIONS)
			{
				device->BindComputePSO(&CPSO_simulate_DEPTHCOLLISIONS, threadID);
			}
			else
			{
				device->BindComputePSO(&CPSO_simulate, threadID);
			}
		}
		device->DispatchIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DISPATCHSIMULATION, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);
		device->EventEnd(threadID);


		device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);
		device->UnBindResources(TEXSLOT_ONDEMAND0, ARRAYSIZE(resources), threadID);

		device->EventEnd(threadID);

	}

	if (SORTING)
	{
#ifdef DEBUG_SORTING
		vector<uint32_t> before(maxCount);
		device->DownloadBuffer(aliveList[1], debugDataReadbackIndexBuffer, before.data(), threadID);

		device->DownloadBuffer(counterBuffer, debugDataReadbackBuffer, &debugData, threadID);
		uint32_t particleCount = debugData.aliveCount_afterSimulation;
#endif // DEBUG_SORTING


		wiGPUSortLib::Sort(MAX_PARTICLES, distanceBuffer, counterBuffer, PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION, aliveList[1], threadID);


#ifdef DEBUG_SORTING
		vector<uint32_t> after(maxCount);
		device->DownloadBuffer(aliveList[1], debugDataReadbackIndexBuffer, after.data(), threadID);

		vector<float> distances(maxCount);
		device->DownloadBuffer(distanceBuffer, debugDataReadbackDistanceBuffer, distances.data(), threadID);

		if (particleCount > 1)
		{
			// CPU sort:
			for (uint32_t i = 0; i < particleCount - 1; ++i)
			{
				for (uint32_t j = i + 1; j < particleCount; ++j)
				{
					uint32_t particleIndexA = before[i];
					uint32_t particleIndexB = before[j];

					float distA = distances[particleIndexA];
					float distB = distances[particleIndexB];

					if (distA > distB)
					{
						before[i] = particleIndexB;
						before[j] = particleIndexA;
					}
				}
			}

			// Validate:
			bool valid = true;
			uint32_t i = 0;
			for (i = 0; i < particleCount; ++i)
			{
				if (before[i] != after[i])
				{
					if (distances[before[i]] != distances[after[i]]) // if distances are equal, we just don't care...
					{
						valid = false;
						break;
					}
				}
			}

			assert(valid && "Invalid GPU sorting result!");

			// Also we can reupload CPU sorted particles to verify:
			if (!valid)
			{
				device->UpdateBuffer(aliveList[1], before.data(), threadID);
			}
		}
#endif // DEBUG_SORTING
	}

	if (!PAUSED)
	{
		// finish updating, update draw argument buffer:
		device->EventBegin("FinishUpdate", threadID);
		device->BindComputePSO(&CPSO_finishUpdate, threadID);

		GPUResource* res[] = {
			counterBuffer,
		};
		device->BindResources(CS, res, 0, ARRAYSIZE(res), threadID);

		GPUResource* uavs[] = {
			indirectBuffers,
		};
		device->BindUnorderedAccessResourcesCS(uavs, 0, ARRAYSIZE(uavs), threadID);

		device->Dispatch(1, 1, 1, threadID);
		device->UAVBarrier(uavs, ARRAYSIZE(uavs), threadID);

		device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);
		device->UnBindResources(0, ARRAYSIZE(res), threadID);
		device->EventEnd(threadID);


		// Swap CURRENT alivelist with NEW alivelist
		SwapPtr(aliveList[0], aliveList[1]);
		emit -= (UINT)emit;
	}

	if (DEBUG)
	{
		device->DownloadBuffer(counterBuffer, debugDataReadbackBuffer, &debugData, threadID);
	}
}


void wiEmittedParticle::Draw(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("EmittedParticle", threadID);

	if (wiRenderer::IsWireRender())
	{
		device->BindGraphicsPSO(&PSO_wire, threadID);
	}
	else
	{
		device->BindGraphicsPSO(&PSO[material->blendFlag][shaderType], threadID);
	}

	device->BindConstantBuffer(VS, constantBuffer, CB_GETBINDSLOT(EmittedParticleCB), threadID);

	GPUResource* res[] = {
		particleBuffer,
		aliveList[0]
	};
	device->TransitionBarrier(res, ARRAYSIZE(res), RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
	device->BindResources(VS, res, 0, ARRAYSIZE(res), threadID);

	if (!wiRenderer::IsWireRender() && material->texture)
	{
		device->BindResource(PS, material->texture, TEXSLOT_ONDEMAND0, threadID);
	}

	device->DrawInstancedIndirect(indirectBuffers, ARGUMENTBUFFER_OFFSET_DRAWPARTICLES, threadID);

	device->EventEnd(threadID);
}


void wiEmittedParticle::CleanUp()
{
	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(aliveList[0]);
	SAFE_DELETE(aliveList[1]);
	SAFE_DELETE(deadList);
	SAFE_DELETE(distanceBuffer);
	SAFE_DELETE(densityBuffer);
	SAFE_DELETE(counterBuffer);
	SAFE_DELETE(indirectBuffers);
	SAFE_DELETE(constantBuffer);
	SAFE_DELETE(debugDataReadbackBuffer);
	SAFE_DELETE(debugDataReadbackIndexBuffer);
	SAFE_DELETE(debugDataReadbackDistanceBuffer);
}



void wiEmittedParticle::LoadShaders()
{
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticleVS.cso", wiResourceManager::VERTEXSHADER));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
	}


	pixelShader[SOFT] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS_soft.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SOFT_DISTORTION] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS_soft_distortion.cso", wiResourceManager::PIXELSHADER));
	pixelShader[SIMPLEST] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));
	
	
	kickoffUpdateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_kickoffUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	finishUpdateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_finishUpdateCS.cso", wiResourceManager::COMPUTESHADER));
	emitCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_emitCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_sphpartitionCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_sphpartitionoffsetsCS.cso", wiResourceManager::COMPUTESHADER));
	sphpartitionoffsetsresetCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_sphpartitionoffsetsresetCS.cso", wiResourceManager::COMPUTESHADER));
	sphdensityCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_sphdensityCS.cso", wiResourceManager::COMPUTESHADER));
	sphforceCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_sphforceCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateCS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateCS_SORTING.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_DEPTHCOLLISIONS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateCS_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));
	simulateCS_SORTING_DEPTHCOLLISIONS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.cso", wiResourceManager::COMPUTESHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	for (int i = 0; i < BLENDMODE_COUNT; ++i)
	{
		GraphicsPSODesc desc;
		desc.vs = vertexShader;
		desc.bs = &blendStates[i];
		desc.rs = &rasterizerState;
		desc.dss = &depthStencilState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;

		desc.ps = pixelShader[SOFT];
		device->CreateGraphicsPSO(&desc, &PSO[i][SOFT]);
		desc.ps = pixelShader[SOFT_DISTORTION];
		device->CreateGraphicsPSO(&desc, &PSO[i][SOFT_DISTORTION]);
		desc.ps = pixelShader[SIMPLEST];
		device->CreateGraphicsPSO(&desc, &PSO[i][SIMPLEST]);
	}

	{
		GraphicsPSODesc desc;
		desc.vs = vertexShader;
		desc.ps = pixelShader[SIMPLEST];
		desc.bs = &blendStates[BLENDMODE_ALPHA];
		desc.rs = &wireFrameRS;
		desc.dss = &depthStencilState;
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::RTFormat_hdr;

		device->CreateGraphicsPSO(&desc, &PSO_wire);
	}

	{
		ComputePSODesc desc;

		desc.cs = kickoffUpdateCS;
		device->CreateComputePSO(&desc, &CPSO_kickoffUpdate);

		desc.cs = finishUpdateCS;
		device->CreateComputePSO(&desc, &CPSO_finishUpdate);

		desc.cs = emitCS;
		device->CreateComputePSO(&desc, &CPSO_emit);

		desc.cs = sphpartitionCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartition);

		desc.cs = sphpartitionoffsetsCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartitionoffsets);

		desc.cs = sphpartitionoffsetsresetCS;
		device->CreateComputePSO(&desc, &CPSO_sphpartitionoffsetsreset);

		desc.cs = sphdensityCS;
		device->CreateComputePSO(&desc, &CPSO_sphdensity);

		desc.cs = sphforceCS;
		device->CreateComputePSO(&desc, &CPSO_sphforce);

		desc.cs = simulateCS;
		device->CreateComputePSO(&desc, &CPSO_simulate);

		desc.cs = simulateCS_SORTING;
		device->CreateComputePSO(&desc, &CPSO_simulate_SORTING);

		desc.cs = simulateCS_DEPTHCOLLISIONS;
		device->CreateComputePSO(&desc, &CPSO_simulate_DEPTHCOLLISIONS);

		desc.cs = simulateCS_SORTING_DEPTHCOLLISIONS;
		device->CreateComputePSO(&desc, &CPSO_simulate_SORTING_DEPTHCOLLISIONS);
	}

}
void wiEmittedParticle::LoadBuffers()
{
}
void wiEmittedParticle::SetUpStates()
{
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,&rasterizerState);

	
	rs.FillMode=FILL_WIREFRAME;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,&wireFrameRS);

	
	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &depthStencilState);

	
	BlendStateDesc bd;
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable=false;
	wiRenderer::GetDevice()->CreateBlendState(&bd,&blendStates[BLENDMODE_ALPHA]);

	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_ONE;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable=false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_ADDITIVE]);

	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_PREMULTIPLIED]);

	bd.RenderTarget[0].BlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bd, &blendStates[BLENDMODE_OPAQUE]);
}
void wiEmittedParticle::SetUpStatic()
{
	LoadBuffers();
	SetUpStates();
	LoadShaders();
}
void wiEmittedParticle::CleanUpStatic()
{
	SAFE_DELETE(vertexShader);
	for (int i = 0; i < ARRAYSIZE(pixelShader); ++i)
	{
		SAFE_DELETE(pixelShader[i]);
	}
	SAFE_DELETE(emitCS);
	SAFE_DELETE(simulateCS);
	SAFE_DELETE(simulateCS_SORTING);
	SAFE_DELETE(simulateCS_DEPTHCOLLISIONS);
	SAFE_DELETE(simulateCS_SORTING_DEPTHCOLLISIONS);
}

void wiEmittedParticle::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> emit;
		if (archive.GetVersion() < 9)
		{
			XMFLOAT4X4 transform4;
			XMFLOAT3X3 transform3;
			archive >> transform4;
			archive >> transform3;
		}
		archive >> name;
		archive >> materialName;
		archive >> size;
		archive >> random_factor;
		archive >> normal_factor;
		archive >> count;
		archive >> life;
		archive >> random_life;
		archive >> scaleX;
		archive >> scaleY;
		archive >> rotation;
		archive >> motionBlurAmount;
		if (archive.GetVersion() < 9)
		{
			string lightName;
			archive >> lightName;
		}
		if (archive.GetVersion() >= 11)
		{
			archive >> MAX_PARTICLES;
			archive >> SORTING;
		}
		if (archive.GetVersion() >= 12)
		{
			archive >> DEPTHCOLLISIONS;
		}
		if (archive.GetVersion() >= 14)
		{
			int tmp;
			archive >> tmp;
			shaderType = (PARTICLESHADERTYPE)tmp;
		}
		if (archive.GetVersion() >= 18)
		{
			archive >> mass;
			archive >> FIXED_TIMESTEP;
			archive >> SPH_FLUIDSIMULATION;
			archive >> SPH_h;
			archive >> SPH_K;
			archive >> SPH_p0;
			archive >> SPH_e;
		}

	}
	else
	{
		archive << emit;
		archive << name;
		archive << materialName;
		archive << size;
		archive << random_factor;
		archive << normal_factor;
		archive << count;
		archive << life;
		archive << random_life;
		archive << scaleX;
		archive << scaleY;
		archive << rotation;
		archive << motionBlurAmount;
		if (archive.GetVersion() >= 11)
		{
			archive << MAX_PARTICLES;
			archive << SORTING;
		}
		if (archive.GetVersion() >= 12)
		{
			archive << DEPTHCOLLISIONS;
		}
		if (archive.GetVersion() >= 14)
		{
			archive << (int)shaderType;
		}
		if (archive.GetVersion() >= 18)
		{
			archive << mass;
			archive << FIXED_TIMESTEP;
			archive << SPH_FLUIDSIMULATION;
			archive << SPH_h;
			archive << SPH_K;
			archive << SPH_p0;
			archive << SPH_e;
		}

	}
}
