#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiMath.h"
#include "wiIntersect.h"
#include "wiRandom.h"
#include "shaders/ResourceMapping.h"
#include "wiArchive.h"
#include "shaders/ShaderInterop.h"
#include "wiTextureHelper.h"
#include "wiScene.h"
#include "shaders/ShaderInterop_HairParticle.h"
#include "wiBackLog.h"
#include "wiEvent.h"

using namespace wiGraphics;

namespace wiScene
{

static Shader vs;
static Shader ps_prepass;
static Shader ps;
static Shader ps_simple;
static Shader cs_simulate;
static Shader cs_finishUpdate;
static DepthStencilState dss_default, dss_equal;
static RasterizerState rs, ncrs, wirers;
static BlendState bs; 
static PipelineState PSO[RENDERPASS_COUNT];
static PipelineState PSO_wire;

void wiHairParticle::UpdateCPU(const TransformComponent& transform, const MeshComponent& mesh, float dt)
{
	world = transform.world;

	XMFLOAT3 _min = mesh.aabb.getMin();
	XMFLOAT3 _max = mesh.aabb.getMax();

	_max.x += length;
	_max.y += length;
	_max.z += length;

	_min.x -= length;
	_min.y -= length;
	_min.z -= length;

	aabb = AABB(_min, _max);
	aabb = aabb.transform(world);

	GraphicsDevice* device = wiRenderer::GetDevice();

	if (_flags & REBUILD_BUFFERS || !cb.IsValid() || (strandCount * segmentCount) != simulationBuffer.GetDesc().Size / sizeof(PatchSimulationData))
	{
		_flags &= ~REBUILD_BUFFERS;
		regenerate_frame = true;

		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;

		if (strandCount * segmentCount > 0)
		{
			bd.Stride = sizeof(PatchSimulationData);
			bd.Size = bd.Stride * strandCount * segmentCount;
			device->CreateBuffer(&bd, nullptr, &simulationBuffer);
			device->SetName(&simulationBuffer, "simulationBuffer");

			bd.MiscFlags = RESOURCE_MISC_BUFFER_RAW;
			bd.Stride = sizeof(MeshComponent::Vertex_POS);
			bd.Size = bd.Stride * 4 * strandCount * segmentCount;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[0]);
			device->SetName(&vertexBuffer_POS[0], "vertexBuffer_POS[0]");
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_POS[1]);
			device->SetName(&vertexBuffer_POS[1], "vertexBuffer_POS[1]");

			bd.Stride = sizeof(MeshComponent::Vertex_TEX);
			bd.Size = bd.Stride * 4 * strandCount * segmentCount;
			device->CreateBuffer(&bd, nullptr, &vertexBuffer_TEX);
			device->SetName(&vertexBuffer_TEX, "vertexBuffer_TEX");

			bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			bd.MiscFlags = RESOURCE_MISC_NONE;
			bd.Format = FORMAT_R32_UINT;
			bd.Stride = sizeof(uint);
			bd.Size = bd.Stride * 6 * strandCount * segmentCount;
			device->CreateBuffer(&bd, nullptr, &primitiveBuffer);
			device->SetName(&primitiveBuffer, "primitiveBuffer");

			bd.BindFlags = BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS;
			bd.MiscFlags = RESOURCE_MISC_NONE;
			bd.Format = FORMAT_R32_UINT;
			bd.Stride = sizeof(uint);
			bd.Size = bd.Stride * 6 * strandCount * segmentCount;
			device->CreateBuffer(&bd, nullptr, &culledIndexBuffer);
			device->SetName(&culledIndexBuffer, "culledIndexBuffer");
		}

		bd.Usage = USAGE_DEFAULT;
		bd.Size = sizeof(HairParticleCB);
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.MiscFlags = RESOURCE_MISC_NONE;
		device->CreateBuffer(&bd, nullptr, &cb);

		if (vertex_lengths.size() != mesh.vertex_positions.size())
		{
			vertex_lengths.resize(mesh.vertex_positions.size());
			std::fill(vertex_lengths.begin(), vertex_lengths.end(), 1.0f);
		}

		indices.clear();
		for (size_t j = 0; j < mesh.indices.size(); j += 3)
		{
			const uint32_t triangle[] = {
				mesh.indices[j + 0],
				mesh.indices[j + 1],
				mesh.indices[j + 2],
			};
			if (vertex_lengths[triangle[0]] > 0 || vertex_lengths[triangle[1]] > 0 || vertex_lengths[triangle[2]] > 0)
			{
				indices.push_back(triangle[0]);
				indices.push_back(triangle[1]);
				indices.push_back(triangle[2]);
			}
		}

		if (!vertex_lengths.empty())
		{
			std::vector<uint8_t> ulengths;
			ulengths.reserve(vertex_lengths.size());
			for (auto& x : vertex_lengths)
			{
				ulengths.push_back(uint8_t(wiMath::Clamp(x, 0, 1) * 255.0f));
			}

			bd.MiscFlags = RESOURCE_MISC_NONE;
			bd.BindFlags = BIND_SHADER_RESOURCE;
			bd.Format = FORMAT_R8_UNORM;
			bd.Stride = sizeof(uint8_t);
			bd.Size = bd.Stride * (uint32_t)ulengths.size();
			device->CreateBuffer(&bd, ulengths.data(), &vertexBuffer_length);
		}
		if (!indices.empty())
		{
			bd.MiscFlags = RESOURCE_MISC_NONE;
			bd.BindFlags = BIND_SHADER_RESOURCE;
			bd.Format = FORMAT_R32_UINT;
			bd.Stride = sizeof(uint32_t);
			bd.Size = bd.Stride * (uint32_t)indices.size();
			device->CreateBuffer(&bd, indices.data(), &indexBuffer);
		}

		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
		{
			RaytracingAccelerationStructureDesc desc;
			desc.type = RaytracingAccelerationStructureDesc::BOTTOMLEVEL;
			desc._flags |= RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE;
			desc._flags |= RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD;

			desc.bottomlevel.geometries.emplace_back();
			auto& geometry = desc.bottomlevel.geometries.back();
			geometry.type = RaytracingAccelerationStructureDesc::BottomLevel::Geometry::TRIANGLES;
			geometry.triangles.vertexBuffer = vertexBuffer_POS[0];
			geometry.triangles.indexBuffer = primitiveBuffer;
			geometry.triangles.indexFormat = INDEXFORMAT_32BIT;
			geometry.triangles.indexCount = (uint32_t)(primitiveBuffer.desc.Size / primitiveBuffer.desc.Stride);
			geometry.triangles.indexOffset = 0;
			geometry.triangles.vertexCount = (uint32_t)(vertexBuffer_POS[0].desc.Size / vertexBuffer_POS[0].desc.Stride);
			geometry.triangles.vertexFormat = FORMAT_R32G32B32_FLOAT;
			geometry.triangles.vertexStride = sizeof(MeshComponent::Vertex_POS);

			bool success = device->CreateRaytracingAccelerationStructure(&desc, &BLAS);
			assert(success);
			device->SetName(&BLAS, "BLAS_hair");
		}
	}

	if (!indirectBuffer.IsValid())
	{
		GPUBufferDesc desc;
		desc.Size = sizeof(uint) + sizeof(IndirectDrawArgsIndexedInstanced); // counter + draw args
		desc.MiscFlags = RESOURCE_MISC_BUFFER_RAW | RESOURCE_MISC_INDIRECT_ARGS;
		desc.BindFlags = BIND_UNORDERED_ACCESS;
		device->CreateBuffer(&desc, nullptr, &indirectBuffer);
	}

	if (!subsetBuffer.IsValid())
	{
		GPUBufferDesc desc;
		desc.Stride = sizeof(ShaderMeshSubset);
		desc.Size = desc.Stride;
		desc.MiscFlags = RESOURCE_MISC_BUFFER_RAW;
		desc.BindFlags = BIND_SHADER_RESOURCE;
		device->CreateBuffer(&desc, nullptr, &subsetBuffer);
	}

	std::swap(vertexBuffer_POS[0], vertexBuffer_POS[1]);

	if (BLAS.IsValid() && !BLAS.desc.bottomlevel.geometries.empty())
	{
		BLAS.desc.bottomlevel.geometries.back().triangles.vertexBuffer = vertexBuffer_POS[0];
	}

}
void wiHairParticle::UpdateGPU(uint32_t instanceIndex, uint32_t materialIndex, const MeshComponent& mesh, const MaterialComponent& material, CommandList cmd) const
{
	if (strandCount == 0 || !simulationBuffer.IsValid())
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - UpdateRenderData", cmd);

	TextureDesc desc;
	if (material.textures[MaterialComponent::BASECOLORMAP].resource != nullptr)
	{
		desc = material.textures[MaterialComponent::BASECOLORMAP].resource->texture.GetDesc();
	}
	HairParticleCB hcb;
	hcb.xHairWorld = world;
	hcb.xHairRegenerate = regenerate_frame ? 1 : 0;
	hcb.xLength = length;
	hcb.xStiffness = stiffness;
	hcb.xHairRandomness = randomness;
	hcb.xHairStrandCount = strandCount;
	hcb.xHairSegmentCount = std::max(segmentCount, 1u);
	hcb.xHairParticleCount = hcb.xHairStrandCount * hcb.xHairSegmentCount;
	hcb.xHairRandomSeed = randomSeed;
	hcb.xHairViewDistance = viewDistance;
	hcb.xHairBaseMeshIndexCount = (indices.empty() ? (uint)mesh.indices.size() : (uint)indices.size());
	hcb.xHairBaseMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
	// segmentCount will be loop in the shader, not a threadgroup so we don't need it here:
	hcb.xHairNumDispatchGroups = (hcb.xHairParticleCount + THREADCOUNT_SIMULATEHAIR - 1) / THREADCOUNT_SIMULATEHAIR;
	hcb.xHairFramesXY = uint2(std::max(1u, framesX), std::max(1u, framesY));
	hcb.xHairFrameCount = std::max(1u, frameCount);
	hcb.xHairFrameStart = frameStart;
	hcb.xHairTexMul = float2(1.0f / (float)hcb.xHairFramesXY.x, 1.0f / (float)hcb.xHairFramesXY.y);
	hcb.xHairAspect = (float)std::max(1u, desc.Width) / (float)std::max(1u, desc.Height);
	hcb.xHairLayerMask = layerMask;
	hcb.xHairInstanceIndex = instanceIndex;
	device->UpdateBuffer(&cb, &hcb, cmd);

	ShaderMeshSubset subset;
	subset.init();
	subset.indexOffset = 0;
	subset.materialIndex = materialIndex;
	device->UpdateBuffer(&subsetBuffer, &subset, cmd);

	{
		GPUBarrier barriers[] = {
			GPUBarrier::Buffer(&cb, RESOURCE_STATE_COPY_DST, RESOURCE_STATE_CONSTANT_BUFFER),
			GPUBarrier::Buffer(&subsetBuffer, RESOURCE_STATE_COPY_DST, RESOURCE_STATE_SHADER_RESOURCE),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);
	}

	// Simulate:
	{
		device->BindComputeShader(&cs_simulate, cmd);
		device->BindConstantBuffer(&cb, CB_GETBINDSLOT(HairParticleCB), cmd);

		const GPUResource* uavs[] = {
			&simulationBuffer,
			&vertexBuffer_POS[0],
			&vertexBuffer_TEX,
			&primitiveBuffer,
			&culledIndexBuffer,
			&indirectBuffer
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		const GPUResource* res[] = {
			indexBuffer.IsValid() ? &indexBuffer : &mesh.indexBuffer,
			mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
			&vertexBuffer_length
		};
		device->BindResources(res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		device->Dispatch(hcb.xHairNumDispatchGroups, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

	}

	// Finish update (reset counter, create indirect draw args):
	{
		device->BindComputeShader(&cs_finishUpdate, cmd);

		const GPUResource* uavs[] = {
			&indirectBuffer
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
			GPUBarrier::Buffer(&indirectBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT),
			GPUBarrier::Buffer(&vertexBuffer_POS[0], RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&vertexBuffer_TEX, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&primitiveBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_SHADER_RESOURCE),
			GPUBarrier::Buffer(&culledIndexBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDEX_BUFFER),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

	}

	device->EventEnd(cmd);

	regenerate_frame = false;
}

void wiHairParticle::Draw(const MaterialComponent& material, RENDERPASS renderPass, CommandList cmd) const
{
	if (strandCount == 0 || !cb.IsValid())
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - Draw", cmd);

	device->BindStencilRef(STENCILREF_DEFAULT, cmd);

	if (wiRenderer::IsWireRender())
	{
		if (renderPass == RENDERPASS_PREPASS)
		{
			return;
		}
		device->BindPipelineState(&PSO_wire, cmd);
		device->BindResource(wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
	}
	else
	{
		device->BindPipelineState(&PSO[renderPass], cmd);

		if (renderPass != RENDERPASS_PREPASS) // depth only alpha test will be full res
		{
			device->BindShadingRate(material.shadingRate, cmd);
		}
	}

	device->BindConstantBuffer(&cb, CB_GETBINDSLOT(HairParticleCB), cmd);
	device->BindResource(&primitiveBuffer, 0, cmd);

	device->BindIndexBuffer(&culledIndexBuffer, INDEXFORMAT_32BIT, 0, cmd);

	device->DrawIndexedInstancedIndirect(&indirectBuffer, 4, cmd);

	device->EventEnd(cmd);
}


void wiHairParticle::Serialize(wiArchive& archive, wiECS::EntitySerializer& seri)
{
	if (archive.IsReadMode())
	{
		archive >> _flags;
		wiECS::SerializeEntity(archive, meshID, seri);
		archive >> strandCount;
		archive >> segmentCount;
		archive >> randomSeed;
		archive >> length;
		archive >> stiffness;
		archive >> randomness;
		archive >> viewDistance;
		if (archive.GetVersion() >= 39)
		{
			archive >> vertex_lengths;
		}
		if (archive.GetVersion() >= 42)
		{
			archive >> framesX;
			archive >> framesY;
			archive >> frameCount;
			archive >> frameStart;
		}

		if (archive.GetVersion() == 48)
		{
			uint8_t shadingRate;
			archive >> shadingRate; // no longer needed
		}
	}
	else
	{
		archive << _flags;
		wiECS::SerializeEntity(archive, meshID, seri);
		archive << strandCount;
		archive << segmentCount;
		archive << randomSeed;
		archive << length;
		archive << stiffness;
		archive << randomness;
		archive << viewDistance;
		if (archive.GetVersion() >= 39)
		{
			archive << vertex_lengths;
		}
		if (archive.GetVersion() >= 42)
		{
			archive << framesX;
			archive << framesY;
			archive << frameCount;
			archive << frameStart;
		}
	}
}

namespace wiHairParticle_Internal
{
	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(VS, vs, "hairparticleVS.cso");

		wiRenderer::LoadShader(PS, ps_simple, "hairparticlePS_simple.cso");
		wiRenderer::LoadShader(PS, ps_prepass, "hairparticlePS_prepass.cso");
		wiRenderer::LoadShader(PS, ps, "hairparticlePS.cso");

		wiRenderer::LoadShader(CS, cs_simulate, "hairparticle_simulateCS.cso");
		wiRenderer::LoadShader(CS, cs_finishUpdate, "hairparticle_finishUpdateCS.cso");

		GraphicsDevice* device = wiRenderer::GetDevice();

		for (int i = 0; i < RENDERPASS_COUNT; ++i)
		{
			if (i == RENDERPASS_PREPASS || i == RENDERPASS_MAIN)
			{
				PipelineStateDesc desc;
				desc.vs = &vs;
				desc.bs = &bs;
				desc.rs = &ncrs;
				desc.dss = &dss_default;
				desc.pt = TRIANGLELIST;

				switch (i)
				{
				case RENDERPASS_PREPASS:
					desc.ps = &ps_prepass;
					break;
				case RENDERPASS_MAIN:
					desc.ps = &ps;
					desc.dss = &dss_equal;
					break;
				}

				device->CreatePipelineState(&desc, &PSO[i]);
			}
		}

		{
			PipelineStateDesc desc;
			desc.vs = &vs;
			desc.ps = &ps_simple;
			desc.bs = &bs;
			desc.rs = &wirers;
			desc.dss = &dss_default;
			desc.pt = TRIANGLELIST;
			device->CreatePipelineState(&desc, &PSO_wire);
		}


	}
}

void wiHairParticle::Initialize()
{

	RasterizerState rsd;
	rsd.FillMode = FILL_SOLID;
	rsd.CullMode = CULL_BACK;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	rs = rsd;

	rsd.FillMode = FILL_SOLID;
	rsd.CullMode = CULL_NONE;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	ncrs = rsd;

	rsd.FillMode = FILL_WIREFRAME;
	rsd.CullMode = CULL_NONE;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	wirers = rsd;


	DepthStencilState dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_GREATER;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	dss_default = dsd;

	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_EQUAL;
	dsd.StencilEnable = false;
	dss_equal = dsd;


	BlendState bld;
	bld.RenderTarget[0].BlendEnable = false;
	bld.AlphaToCoverageEnable = false; // maybe for msaa
	bs = bld;

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiHairParticle_Internal::LoadShaders(); });
	wiHairParticle_Internal::LoadShaders();

	wiBackLog::post("wiHairParticle Initialized");
}

}
