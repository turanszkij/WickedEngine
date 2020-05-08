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

using namespace std;
using namespace wiGraphics;

namespace wiScene
{

static Shader vs;
static Shader ps_alphatestonly;
static Shader ps_deferred;
static Shader ps_forward;
static Shader ps_forward_transparent;
static Shader ps_tiledforward;
static Shader ps_tiledforward_transparent;
static Shader ps_simplest;
static Shader cs_simulate;
static DepthStencilState dss_default, dss_equal, dss_rejectopaque_keeptransparent;
static RasterizerState rs, ncrs, wirers;
static BlendState bs[2]; 
static PipelineState PSO[RENDERPASS_COUNT][2];
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
		_flags &= ~REGENERATE_FRAME;
		if (_flags & REBUILD_BUFFERS || !cb.IsValid() || (strandCount * segmentCount) != particleBuffer.GetDesc().ByteWidth / sizeof(Patch))
		{
			_flags &= ~REBUILD_BUFFERS;
			_flags |= REGENERATE_FRAME;

			GraphicsDevice* device = wiRenderer::GetDevice();

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

	device->BindComputeShader(&cs_simulate, cmd);

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

	device->BindConstantBuffer(CS, &cb, CB_GETBINDSLOT(HairParticleCB), cmd);

	const GPUResource* uavs[] = {
		&particleBuffer,
		&simulationBuffer
	};
	device->BindUAVs(CS, uavs, 0, arraysize(uavs), cmd);

	const GPUResource* res[] = {
		indexBuffer.IsValid() ? &indexBuffer : &mesh.indexBuffer,
		mesh.streamoutBuffer_POS.IsValid() ? &mesh.streamoutBuffer_POS : &mesh.vertexBuffer_POS,
		&vertexBuffer_length
	};
	device->BindResources(CS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);

	device->Dispatch(hcb.xHairNumDispatchGroups, 1, 1, cmd);

	device->UnbindUAVs(0, arraysize(uavs), cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, arraysize(res), cmd);

	device->EventEnd(cmd);
}

void wiHairParticle::Draw(const CameraComponent& camera, const MaterialComponent& material, RENDERPASS renderPass, bool transparent, CommandList cmd) const
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
		if (transparent || renderPass == RENDERPASS_DEPTHONLY)
		{
			return;
		}
		device->BindPipelineState(&PSO_wire, cmd);
		device->BindResource(VS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
	}
	else
	{
		device->BindPipelineState(&PSO[renderPass][transparent], cmd);

		const GPUResource* res[] = {
			material.GetBaseColorMap()
		};
		device->BindResources(PS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);
		device->BindResources(VS, res, TEXSLOT_ONDEMAND0, arraysize(res), cmd);
	}

	device->BindConstantBuffer(VS, &cb, CB_GETBINDSLOT(HairParticleCB), cmd);

	device->BindResource(VS, &particleBuffer, 0, cmd);

	device->Draw(strandCount * 12 * std::max(segmentCount, 1u), 0, cmd);

	device->EventEnd(cmd);
}


void wiHairParticle::Serialize(wiArchive& archive, wiECS::Entity seed)
{
	if (archive.IsReadMode())
	{
		archive >> _flags;
		wiECS::SerializeEntity(archive, meshID, seed);
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
	}
	else
	{
		archive << _flags;
		wiECS::SerializeEntity(archive, meshID, seed);
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


void wiHairParticle::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	wiRenderer::LoadShader(VS, vs, "hairparticleVS.cso");

	wiRenderer::LoadShader(PS, ps_simplest, "hairparticlePS_simplest.cso");
	wiRenderer::LoadShader(PS, ps_alphatestonly, "hairparticlePS_alphatestonly.cso");
	wiRenderer::LoadShader(PS, ps_deferred, "hairparticlePS_deferred.cso");
	wiRenderer::LoadShader(PS, ps_forward, "hairparticlePS_forward.cso");
	wiRenderer::LoadShader(PS, ps_forward_transparent, "hairparticlePS_forward_transparent.cso");
	wiRenderer::LoadShader(PS, ps_tiledforward, "hairparticlePS_tiledforward.cso");
	wiRenderer::LoadShader(PS, ps_tiledforward_transparent, "hairparticlePS_tiledforward_transparent.cso");

	wiRenderer::LoadShader(CS, cs_simulate, "hairparticle_simulateCS.cso");

	GraphicsDevice* device = wiRenderer::GetDevice();

	for (int i = 0; i < RENDERPASS_COUNT; ++i)
	{
		if (i == RENDERPASS_DEPTHONLY || i == RENDERPASS_DEFERRED || i == RENDERPASS_FORWARD || i == RENDERPASS_TILEDFORWARD)
		{
			for (int j = 0; j < 2; ++j)
			{
				if ((i == RENDERPASS_DEPTHONLY || i == RENDERPASS_DEFERRED) && j == 1)
				{
					continue;
				}

				PipelineStateDesc desc;
				desc.vs = &vs;
				desc.bs = &bs[j];
				desc.rs = &ncrs;
				desc.dss = &dss_default;

				switch (i)
				{
				case RENDERPASS_DEPTHONLY:
					desc.ps = &ps_alphatestonly;
					break;
				case RENDERPASS_DEFERRED:
					desc.ps = &ps_deferred;
					break;
				case RENDERPASS_FORWARD:
					if (j == 0)
					{
						desc.ps = &ps_forward;
						desc.dss = &dss_equal;
					}
					else
					{
						desc.ps = &ps_forward_transparent;
					}
					break;
				case RENDERPASS_TILEDFORWARD:
					if (j == 0)
					{
						desc.ps = &ps_tiledforward;
						desc.dss = &dss_equal;
					}
					else
					{
						desc.ps = &ps_tiledforward_transparent;
					}
					break;
				}

				if (j == 1)
				{
					desc.dss = &dss_rejectopaque_keeptransparent; // transparent
				}

				device->CreatePipelineState(&desc, &PSO[i][j]);
			}
		}
	}

	{
		PipelineStateDesc desc;
		desc.vs = &vs;
		desc.ps = &ps_simplest;
		desc.bs = &bs[0];
		desc.rs = &wirers;
		desc.dss = &dss_default;
		device->CreatePipelineState(&desc, &PSO_wire);
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
	dsd.DepthFunc = COMPARISON_GREATER;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, &dss_rejectopaque_keeptransparent);


	BlendStateDesc bld;
	bld.RenderTarget[0].BlendEnable = false;
	bld.AlphaToCoverageEnable = false; // maybe for msaa
	wiRenderer::GetDevice()->CreateBlendState(&bld, &bs[0]);

	bld.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bld.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bld.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bld.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bld.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bld.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bld.RenderTarget[0].BlendEnable = true;
	bld.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bld.AlphaToCoverageEnable = false;
	bld.IndependentBlendEnable = false;
	wiRenderer::GetDevice()->CreateBlendState(&bld, &bs[1]);

	LoadShaders();

	wiBackLog::post("wiHairParticle Initialized");
}

}
