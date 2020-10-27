#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiMath.h"
#include "wiIntersect.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "ShaderInterop.h"
#include "wiTextureHelper.h"
#include "wiScene.h"
#include "ShaderInterop_HairParticle.h"
#include "wiBackLog.h"
#include "wiEvent.h"

using namespace std;
using namespace wiGraphics;

namespace wiScene
{

static Shader vs;
static Shader ps_alphatestonly;
static Shader ps;
static Shader ps_simplest;
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

	if (dt > 0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		_flags &= ~REGENERATE_FRAME;
		if (_flags & REBUILD_BUFFERS || !cb.IsValid() || (strandCount * segmentCount) != particleBuffer.GetDesc().ByteWidth / sizeof(Patch))
		{
			_flags &= ~REBUILD_BUFFERS;
			_flags |= REGENERATE_FRAME;

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;

			if (strandCount*segmentCount > 0)
			{
				bd.StructureByteStride = sizeof(Patch);
				bd.ByteWidth = bd.StructureByteStride * strandCount * segmentCount;
				device->CreateBuffer(&bd, nullptr, &particleBuffer);

				bd.StructureByteStride = sizeof(PatchSimulationData);
				bd.ByteWidth = bd.StructureByteStride * strandCount * segmentCount;
				device->CreateBuffer(&bd, nullptr, &simulationBuffer);

				bd.StructureByteStride = sizeof(uint);
				bd.ByteWidth = bd.StructureByteStride * strandCount * segmentCount;
				device->CreateBuffer(&bd, nullptr, &culledIndexBuffer);
			}

			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(HairParticleCB);
			bd.BindFlags = BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = 0;
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

				bd.MiscFlags = 0;
				bd.BindFlags = BIND_SHADER_RESOURCE;
				bd.Format = FORMAT_R8_UNORM;
				bd.StructureByteStride = sizeof(uint8_t);
				bd.ByteWidth = bd.StructureByteStride * (uint32_t)ulengths.size();
				SubresourceData initData;
				initData.pSysMem = ulengths.data();
				device->CreateBuffer(&bd, &initData, &vertexBuffer_length);
			}
			if (!indices.empty())
			{
				bd.MiscFlags = 0;
				bd.BindFlags = BIND_SHADER_RESOURCE;
				bd.Format = FORMAT_R32_UINT;
				bd.StructureByteStride = sizeof(uint32_t);
				bd.ByteWidth = bd.StructureByteStride * (uint32_t)indices.size();
				SubresourceData initData;
				initData.pSysMem = indices.data();
				device->CreateBuffer(&bd, &initData, &indexBuffer);
			}

		}

		if (!indirectBuffer.IsValid())
		{
			GPUBufferDesc desc;
			desc.ByteWidth = sizeof(uint) + sizeof(IndirectDrawArgsInstanced); // counter + draw args
			desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | RESOURCE_MISC_INDIRECT_ARGS;
			desc.BindFlags = BIND_UNORDERED_ACCESS;
			device->CreateBuffer(&desc, nullptr, &indirectBuffer);
		}
	}

}
void wiHairParticle::UpdateGPU(const MeshComponent& mesh, const MaterialComponent& material, CommandList cmd) const
{
	if (strandCount == 0 || !particleBuffer.IsValid())
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - UpdateRenderData", cmd);

	const TextureDesc& desc = material.GetBaseColorMap()->GetDesc();

	HairParticleCB hcb;
	hcb.xWorld = world;
	hcb.xColor = material.baseColor;
	hcb.xHairRegenerate = (_flags & REGENERATE_FRAME) ? 1 : 0;
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
	hcb.xHairAspect = (float)desc.Width / (float)desc.Height;
	device->UpdateBuffer(&cb, &hcb, cmd);

	// Simulate:
	{
		device->BindComputeShader(&cs_simulate, cmd);
		device->BindConstantBuffer(CS, &cb, CB_GETBINDSLOT(HairParticleCB), cmd);

		const GPUResource* uavs[] = {
			&particleBuffer,
			&simulationBuffer,
			&culledIndexBuffer,
			&indirectBuffer
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		const GPUResource* res[] = {
			indexBuffer.IsValid() ? &indexBuffer : &mesh.indexBuffer,
			mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
			&vertexBuffer_length
		};
		device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		device->Dispatch(hcb.xHairNumDispatchGroups, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
		device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(res), cmd);
	}

	// Finish update (reset counter, create indirect draw args):
	{
		device->BindComputeShader(&cs_finishUpdate, cmd);

		const GPUResource* uavs[] = {
			&indirectBuffer
		};
		device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

		device->Dispatch(1, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory()
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		device->UnbindUAVs(0, arraysize(uavs), cmd);
	}

	device->EventEnd(cmd);
}

void wiHairParticle::Draw(const CameraComponent& camera, const MaterialComponent& material, RENDERPASS renderPass, CommandList cmd) const
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
		if (renderPass == RENDERPASS_DEPTHONLY)
		{
			return;
		}
		device->BindPipelineState(&PSO_wire, cmd);
		device->BindResource(VS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
	}
	else
	{
		device->BindPipelineState(&PSO[renderPass], cmd);

		const GPUResource* res[] = {
			material.GetBaseColorMap()
		};
		device->BindResources(PS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);
		device->BindResources(VS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

		if (renderPass != RENDERPASS_DEPTHONLY) // depth only alpha test will be full res
		{
			device->BindShadingRate(material.shadingRate, cmd);
		}
	}

	device->BindConstantBuffer(VS, &cb, CB_GETBINDSLOT(HairParticleCB), cmd);

	device->BindResource(VS, &particleBuffer, 0, cmd);
	device->BindResource(VS, &culledIndexBuffer, 1, cmd);

	device->DrawInstancedIndirect(&indirectBuffer, 4, cmd);

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

		wiRenderer::LoadShader(PS, ps_simplest, "hairparticlePS_simplest.cso");
		wiRenderer::LoadShader(PS, ps_alphatestonly, "hairparticlePS_alphatestonly.cso");
		wiRenderer::LoadShader(PS, ps, "hairparticlePS.cso");

		wiRenderer::LoadShader(CS, cs_simulate, "hairparticle_simulateCS.cso");
		wiRenderer::LoadShader(CS, cs_finishUpdate, "hairparticle_finishUpdateCS.cso");

		GraphicsDevice* device = wiRenderer::GetDevice();

		for (int i = 0; i < RENDERPASS_COUNT; ++i)
		{
			if (i == RENDERPASS_DEPTHONLY || i == RENDERPASS_MAIN)
			{
				PipelineStateDesc desc;
				desc.vs = &vs;
				desc.bs = &bs;
				desc.rs = &ncrs;
				desc.dss = &dss_default;
				desc.pt = TRIANGLESTRIP;

				switch (i)
				{
				case RENDERPASS_DEPTHONLY:
					desc.ps = &ps_alphatestonly;
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
			desc.ps = &ps_simplest;
			desc.bs = &bs;
			desc.rs = &wirers;
			desc.dss = &dss_default;
			device->CreatePipelineState(&desc, &PSO_wire);
		}


	}
}

void wiHairParticle::Initialize()
{

	RasterizerStateDesc rsd;
	rsd.FillMode = FILL_SOLID;
	rsd.CullMode = CULL_BACK;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd, &rs);

	rsd.FillMode = FILL_SOLID;
	rsd.CullMode = CULL_NONE;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd, &ncrs);

	rsd.FillMode = FILL_WIREFRAME;
	rsd.CullMode = CULL_NONE;
	rsd.FrontCounterClockwise = true;
	rsd.DepthBias = 0;
	rsd.DepthBiasClamp = 0;
	rsd.SlopeScaledDepthBias = 0;
	rsd.DepthClipEnable = true;
	rsd.MultisampleEnable = false;
	rsd.AntialiasedLineEnable = false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd, &wirers);


	DepthStencilStateDesc dsd;
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
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &dss_default);

	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_EQUAL;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &dss_equal);


	BlendStateDesc bld;
	bld.RenderTarget[0].BlendEnable = false;
	bld.AlphaToCoverageEnable = false; // maybe for msaa
	wiRenderer::GetDevice()->CreateBlendState(&bld, &bs);

	static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { wiHairParticle_Internal::LoadShaders(); });
	wiHairParticle_Internal::LoadShaders();

	wiBackLog::post("wiHairParticle Initialized");
}

}
