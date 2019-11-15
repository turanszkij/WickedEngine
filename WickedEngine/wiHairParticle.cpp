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
#include "wiSceneSystem.h"
#include "ShaderInterop_HairParticle.h"
#include "wiBackLog.h"

using namespace std;
using namespace wiGraphics;

namespace wiSceneSystem
{

static const VertexShader *vs = nullptr;
static const PixelShader* ps_alphatestonly = nullptr;
static const PixelShader* ps_deferred = nullptr;
static const PixelShader* ps_forward = nullptr;
static const PixelShader* ps_forward_transparent = nullptr;
static const PixelShader* ps_tiledforward = nullptr;
static const PixelShader* ps_tiledforward_transparent = nullptr;
static const PixelShader *ps_simplest = nullptr;
static const ComputeShader *cs_simulate = nullptr;
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
		if (cb == nullptr || (strandCount * segmentCount) != particleBuffer->GetDesc().ByteWidth / sizeof(Patch))
		{
			_flags |= REGENERATE_FRAME;

			cb.reset(new GPUBuffer);
			particleBuffer.reset(new GPUBuffer);
			simulationBuffer.reset(new GPUBuffer);

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;

			if (strandCount*segmentCount > 0)
			{
				bd.StructureByteStride = sizeof(Patch);
				bd.ByteWidth = bd.StructureByteStride * strandCount * segmentCount;
				wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, particleBuffer.get());

				bd.StructureByteStride = sizeof(PatchSimulationData);
				bd.ByteWidth = bd.StructureByteStride * strandCount * segmentCount;
				wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, simulationBuffer.get());
			}

			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(HairParticleCB);
			bd.BindFlags = BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = 0;
			wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, cb.get());
		}
	}

}
void wiHairParticle::UpdateGPU(const MeshComponent& mesh, const MaterialComponent& material, CommandList cmd) const
{
	if (strandCount == 0 || particleBuffer == nullptr)
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - UpdateRenderData", cmd);

	device->BindComputeShader(cs_simulate, cmd);

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
	hcb.xHairBaseMeshIndexCount = (uint)mesh.indices.size();
	hcb.xHairBaseMeshVertexPositionStride = sizeof(MeshComponent::Vertex_POS);
	// segmentCount will be loop in the shader, not a threadgroup so we don't need it here:
	hcb.xHairNumDispatchGroups = (uint)ceilf((float)strandCount / (float)THREADCOUNT_SIMULATEHAIR);
	device->UpdateBuffer(cb.get(), &hcb, cmd);

	device->BindConstantBuffer(CS, cb.get(), CB_GETBINDSLOT(HairParticleCB), cmd);

	GPUResource* uavs[] = {
		particleBuffer.get(),
		simulationBuffer.get()
	};
	device->BindUAVs(CS, uavs, 0, ARRAYSIZE(uavs), cmd);

	GPUResource* res[] = {
		mesh.indexBuffer.get(),
		mesh.streamoutBuffer_POS != nullptr ? mesh.streamoutBuffer_POS.get() : mesh.vertexBuffer_POS.get(),
	};
	device->BindResources(CS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);

	device->Dispatch(hcb.xHairNumDispatchGroups, 1, 1, cmd);

	device->UnbindUAVs(0, ARRAYSIZE(uavs), cmd);
	device->UnbindResources(TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);

	device->EventEnd(cmd);
}

void wiHairParticle::Draw(const CameraComponent& camera, const MaterialComponent& material, RENDERPASS renderPass, bool transparent, CommandList cmd) const
{
	if (strandCount == 0 || cb == nullptr)
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
		device->BindResources(PS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
		device->BindResources(VS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), cmd);
	}

	device->BindConstantBuffer(VS, cb.get(), CB_GETBINDSLOT(HairParticleCB), cmd);

	device->BindResource(VS, particleBuffer.get(), 0, cmd);

	device->Draw(strandCount * 12 * std::max(segmentCount, 1u), 0, cmd);

	device->EventEnd(cmd);
}


void wiHairParticle::Serialize(wiArchive& archive, uint32_t seed)
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
	}
}


void wiHairParticle::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	vs = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticleVS.cso", wiResourceManager::VERTEXSHADER));

	ps_alphatestonly = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	ps_deferred = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_deferred.cso", wiResourceManager::PIXELSHADER));
	ps_forward = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_forward.cso", wiResourceManager::PIXELSHADER));
	ps_forward_transparent = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_forward_transparent.cso", wiResourceManager::PIXELSHADER));
	ps_tiledforward = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_tiledforward.cso", wiResourceManager::PIXELSHADER));
	ps_tiledforward_transparent = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_tiledforward_transparent.cso", wiResourceManager::PIXELSHADER));


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
				desc.vs = vs;
				desc.bs = &bs[j];
				desc.rs = &ncrs;
				desc.dss = &dss_default;

				switch (i)
				{
				case RENDERPASS_DEPTHONLY:
					desc.ps = ps_alphatestonly;
					break;
				case RENDERPASS_DEFERRED:
					desc.ps = ps_deferred;
					break;
				case RENDERPASS_FORWARD:
					if (j == 0)
					{
						desc.ps = ps_forward;
						desc.dss = &dss_equal;
					}
					else
					{
						desc.ps = ps_forward_transparent;
					}
					break;
				case RENDERPASS_TILEDFORWARD:
					if (j == 0)
					{
						desc.ps = ps_tiledforward;
						desc.dss = &dss_equal;
					}
					else
					{
						desc.ps = ps_tiledforward_transparent;
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

	ps_simplest = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));

	{
		PipelineStateDesc desc;
		desc.vs = vs;
		desc.ps = ps_simplest;
		desc.bs = &bs[0];
		desc.rs = &wirers;
		desc.dss = &dss_default;
		device->CreatePipelineState(&desc, &PSO_wire);
	}

	cs_simulate = static_cast<const ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "hairparticle_simulateCS.cso", wiResourceManager::COMPUTESHADER));

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
