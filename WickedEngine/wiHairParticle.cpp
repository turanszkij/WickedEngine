#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiMath.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "ShaderInterop.h"
#include "wiTextureHelper.h"
#include "wiSceneSystem.h"

using namespace std;
using namespace wiGraphicsTypes;

namespace wiSceneSystem
{

VertexShader *wiHairParticle::vs = nullptr;
PixelShader *wiHairParticle::ps[];
PixelShader *wiHairParticle::ps_simplest = nullptr;
DepthStencilState wiHairParticle::dss_default, wiHairParticle::dss_equal, wiHairParticle::dss_rejectopaque_keeptransparent;
RasterizerState wiHairParticle::rs, wiHairParticle::ncrs, wiHairParticle::wirers;
BlendState wiHairParticle::bs[2]; 
GraphicsPSO wiHairParticle::PSO[SHADERTYPE_COUNT][2];
GraphicsPSO wiHairParticle::PSO_wire;
int wiHairParticle::LOD[3];

void wiHairParticle::CleanUpStatic()
{
	SAFE_DELETE(vs);
	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_DELETE(ps[i]);
	}
}
void wiHairParticle::LoadShaders()
{
	vs = static_cast<VertexShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticleVS.cso", wiResourceManager::VERTEXSHADER));
	

	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_INIT(ps[i]);
	}
	
	ps[SHADERTYPE_DEPTHONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticlePS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticlePS_deferred.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_FORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticlePS_forward.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_TILEDFORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticlePS_tiledforward.cso", wiResourceManager::PIXELSHADER));


	GraphicsDevice* device = wiRenderer::GetDevice();

	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		if (ps[i] == nullptr)
		{
			continue;
		}

		for (int j = 0; j < 2; ++j)
		{
			if ((i == SHADERTYPE_DEPTHONLY || i == SHADERTYPE_DEFERRED) && j == 1)
			{
				continue;
			}

			GraphicsPSODesc desc;
			desc.vs = vs;
			desc.ps = ps[i];
			desc.bs = &bs[j];
			desc.rs = &ncrs;
			desc.dss = &dss_default;

			desc.DSFormat = wiRenderer::DSFormat_full;

			switch (i)
			{
			case SHADERTYPE_TEXTURE:
				desc.numRTs = 1;
				desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
				break;
			case SHADERTYPE_FORWARD:
			case SHADERTYPE_TILEDFORWARD:
				desc.numRTs = 2;
				desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
				desc.RTFormats[1] = wiRenderer::RTFormat_gbuffer_1;
				break;
			case SHADERTYPE_DEFERRED:
				desc.numRTs = 4;
				desc.RTFormats[0] = wiRenderer::RTFormat_gbuffer_0;
				desc.RTFormats[1] = wiRenderer::RTFormat_gbuffer_1;
				desc.RTFormats[2] = wiRenderer::RTFormat_gbuffer_2;
				desc.RTFormats[3] = wiRenderer::RTFormat_gbuffer_3;
			default:
				break;
			}

			if (i == SHADERTYPE_TILEDFORWARD)
			{
				desc.dss = &dss_equal; // opaque
			}

			if(j == 1)
			{
				desc.dss = &dss_rejectopaque_keeptransparent; // transparent
				desc.numRTs = 1;
			}

			device->CreateGraphicsPSO(&desc, &PSO[i][j]);
		}
	}

	SAFE_INIT(ps_simplest);
	ps_simplest = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "hairparticlePS_simplest.cso", wiResourceManager::PIXELSHADER));

	GraphicsPSODesc desc;
	desc.vs = vs;
	desc.ps = ps_simplest;
	desc.bs = &bs[0];
	desc.rs = &wirers;
	desc.dss = &dss_default;
	desc.numRTs = 1;
	desc.RTFormats[0] = wiRenderer::RTFormat_hdr;
	desc.DSFormat = wiRenderer::DSFormat_full;
	device->CreateGraphicsPSO(&desc, &PSO_wire);
}
void wiHairParticle::SetUpStatic()
{
	Settings(10,25,120);


	RasterizerStateDesc rsd;
	rsd.FillMode=FILL_SOLID;
	rsd.CullMode=CULL_BACK;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd, &rs);

	rsd.FillMode=FILL_SOLID;
	rsd.CullMode=CULL_NONE;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
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
	bld.RenderTarget[0].BlendEnable=false;
	bld.AlphaToCoverageEnable=false; // maybe for msaa
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
}
void wiHairParticle::Settings(int l0,int l1,int l2)
{
	LOD[0]=l0;
	LOD[1]=l1;
	LOD[2]=l2;
}


void wiHairParticle::Generate(const MeshComponent& mesh)
{
	std::vector<Patch> points(particleCount);

	// Now the distribution is completely uniform. TODO: bring back distribution, but make it more intuitive to set up!

	for (UINT i = 0; i < particleCount; ++i)
	{
		int tri = wiRandom::getRandom(0, (int)((mesh.indices.size() - 1) / 3));

		uint32_t i0 = mesh.indices[tri * 3 + 0];
		uint32_t i1 = mesh.indices[tri * 3 + 1];
		uint32_t i2 = mesh.indices[tri * 3 + 2];

		XMVECTOR p0 = XMLoadFloat3(&mesh.vertex_positions[i0]);
		XMVECTOR p1 = XMLoadFloat3(&mesh.vertex_positions[i1]);
		XMVECTOR p2 = XMLoadFloat3(&mesh.vertex_positions[i2]);

		XMVECTOR n0 = XMLoadFloat3(&mesh.vertex_normals[i0]);
		XMVECTOR n1 = XMLoadFloat3(&mesh.vertex_normals[i1]);
		XMVECTOR n2 = XMLoadFloat3(&mesh.vertex_normals[i2]);

		float f = wiRandom::getRandom(0, 1000) * 0.001f;
		float g = wiRandom::getRandom(0, 1000) * 0.001f;

		XMVECTOR P = XMVectorBaryCentric(p0, p1, p2, f, g);
		XMVECTOR N = XMVectorBaryCentric(n0, n1, n2, f, g);
		XMVECTOR T = XMVector3Normalize(XMVectorSubtract(p0, p1));

		XMStoreFloat4(&points[i].posLen, P);
		points[i].posLen.w = 1.0f;

		XMFLOAT3 normal, tangent;
		XMStoreFloat3(&normal, N);
		XMStoreFloat3(&tangent, T);

		points[i].normalRand = wiMath::CompressNormal(normal);
		points[i].tangent = wiMath::CompressNormal(tangent);
	}

	cb.reset(new GPUBuffer);
	particleBuffer.reset(new GPUBuffer);

	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_IMMUTABLE;
	bd.ByteWidth = (UINT)(sizeof(Patch) * particleCount);
	bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	SubresourceData data = {};
	data.pSysMem = points.data();
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, particleBuffer.get());

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, cb.get());

}

void wiHairParticle::Draw(CameraComponent* camera, const MaterialComponent& material, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID) const
{
	if (cb == nullptr)
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - Draw", threadID);

	if (wiRenderer::IsWireRender())
	{
		if (transparent || shaderType == SHADERTYPE_DEPTHONLY)
		{
			return;
		}
		device->BindGraphicsPSO(&PSO_wire, threadID);
		device->BindResource(VS, wiTextureHelper::getInstance()->getWhite(), TEXSLOT_ONDEMAND0, threadID);
	}
	else
	{
		device->BindGraphicsPSO(&PSO[shaderType][transparent], threadID);

		GPUResource* res[] = {
			material.GetBaseColorMap()
		};
		device->BindResources(PS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);
		device->BindResources(VS, res, TEXSLOT_ONDEMAND0, ARRAYSIZE(res), threadID);
	}

	ConstantBuffer gcb;
	gcb.mWorld = XMMatrixTranspose(XMLoadFloat4x4(&world));
	gcb.color = material.baseColor;
	gcb.LOD0 = (float)LOD[0];
	gcb.LOD1 = (float)LOD[1];
	gcb.LOD2 = (float)LOD[2];
	device->UpdateBuffer(cb.get(), &gcb, threadID);

	device->BindConstantBuffer(VS, cb.get(), CB_GETBINDSLOT(ConstantBuffer), threadID);

	device->BindResource(VS, particleBuffer.get(), 0, threadID);

	device->Draw((int)particleCount * 12, 0, threadID);

	device->EventEnd(threadID);
}

}
