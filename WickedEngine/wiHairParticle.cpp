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


void wiHairParticle::Generate()
{
	//std::vector<Patch> points;

	//Mesh* mesh = object->mesh;

	//XMMATRIX matr = object->getMatrix();
	//XMStoreFloat4x4(&OriginalMatrix_Inverse, XMMatrixInverse(nullptr, matr));

	//int dVG = -1, lVG = -1;
	//if (densityG.compare("")) {
	//	for (unsigned int i = 0; i < mesh->vertexGroups.size(); ++i)
	//		if (!mesh->vertexGroups[i].name.compare(densityG))
	//			dVG = i;
	//}
	//if (lenG.compare("")) {
	//	for (unsigned int i = 0; i < mesh->vertexGroups.size(); ++i)
	//		if (!mesh->vertexGroups[i].name.compare(lenG))
	//			lVG = i;
	//}
	//
	//float avgPatchSize;
	//if(dVG>=0)
	//	avgPatchSize = (float)count/((float)mesh->vertexGroups[dVG].vertices.size()/3.0f);
	//else
	//	avgPatchSize = (float)count/((float)mesh->indices.size()/3.0f);

	//if (mesh->indices.size() < 4)
	//	return;

	//for (unsigned int i = 0; i<mesh->indices.size() - 3; i += 3)
	//{

	//	unsigned int vi[]={mesh->indices[i],mesh->indices[i+1],mesh->indices[i+2]};
	//	float denMod[]={1,1,1},lenMod[]={1,1,1};
	//	if (dVG >= 0) {
	//		auto found = mesh->vertexGroups[dVG].vertices.find(vi[0]);
	//		if (found != mesh->vertexGroups[dVG].vertices.end())
	//			denMod[0] = found->second;
	//		else
	//			continue;

	//		found = mesh->vertexGroups[dVG].vertices.find(vi[1]);
	//		if (found != mesh->vertexGroups[dVG].vertices.end())
	//			denMod[1] = found->second;
	//		else
	//			continue;

	//		found = mesh->vertexGroups[dVG].vertices.find(vi[2]);
	//		if (found != mesh->vertexGroups[dVG].vertices.end())
	//			denMod[2] = found->second;
	//		else
	//			continue;
	//	}
	//	if (lVG >= 0) {
	//		auto found = mesh->vertexGroups[lVG].vertices.find(vi[0]);
	//		if (found != mesh->vertexGroups[lVG].vertices.end())
	//			lenMod[0] = found->second;
	//		else
	//			continue;

	//		found = mesh->vertexGroups[lVG].vertices.find(vi[1]);
	//		if (found != mesh->vertexGroups[lVG].vertices.end())
	//			lenMod[1] = found->second;
	//		else
	//			continue;

	//		found = mesh->vertexGroups[lVG].vertices.find(vi[2]);
	//		if (found != mesh->vertexGroups[lVG].vertices.end())
	//			lenMod[2] = found->second;
	//		else
	//			continue;
	//	}
	//	for (int m = 0; m < 3; ++m) {
	//		if (denMod[m] < 0) denMod[m] = 0;
	//		if (lenMod[m] < 0) lenMod[m] = 0;
	//	}

	//	Mesh::Vertex_FULL verts[] = {
	//		mesh->vertices_FULL[vi[0]],
	//		mesh->vertices_FULL[vi[1]],
	//		mesh->vertices_FULL[vi[2]],
	//	};

	//	if(
	//		(denMod[0]>FLT_EPSILON || denMod[1]>FLT_EPSILON || denMod[2]>FLT_EPSILON) &&
	//		(lenMod[0]>FLT_EPSILON || lenMod[1]>FLT_EPSILON || lenMod[2]>FLT_EPSILON)
	//	  )
	//	{

	//		float density = (float)(denMod[0]+denMod[1]+denMod[2])/3.0f*avgPatchSize;
	//		int rdense = (int)(( density - (int)density ) * 100);
	//		density += ((wiRandom::getRandom(0, 99)) <= rdense ? 1.0f : 0.0f);
	//		int PATCHSIZE = material->texture?(int)density:(int)density*10;
	//		  
	//		if(PATCHSIZE)
	//		{

	//			for(int p=0;p<PATCHSIZE;++p)
	//			{
	//				float f = wiRandom::getRandom(0, 1000) * 0.001f, g = wiRandom::getRandom(0, 1000) * 0.001f;
	//				if (f + g > 1)
	//				{
	//					f = 1 - f;
	//					g = 1 - g;
	//				}
	//				XMVECTOR pos[] = {
	//					XMVector3Transform(XMLoadFloat4(&verts[0].pos),matr)
	//					,	XMVector3Transform(XMLoadFloat4(&verts[1].pos),matr)
	//					,	XMVector3Transform(XMLoadFloat4(&verts[2].pos),matr)
	//				};
	//				XMVECTOR vbar=XMVectorBaryCentric(
	//						pos[0],pos[1],pos[2]
	//					,	f
	//					,	g
	//					);
	//				XMVECTOR nbar=XMVectorBaryCentric(
	//						XMLoadFloat4(&verts[0].nor)
	//					,	XMLoadFloat4(&verts[1].nor)
	//					,	XMLoadFloat4(&verts[2].nor)
	//					,	f
	//					,	g
	//					);
	//				int ti = wiRandom::getRandom(0, 2);
	//				XMVECTOR tangent = XMVector3Normalize(XMVectorSubtract(pos[ti], pos[(ti + 1) % 3]));
	//				
	//				Patch addP;
	//				::XMStoreFloat4(&addP.posLen,vbar);

	//				XMFLOAT3 nor, tan;
	//				::XMStoreFloat3(&nor,XMVector3Normalize(nbar));
	//				::XMStoreFloat3(&tan,tangent);

	//				addP.normalRand = wiMath::CompressNormal(nor);
	//				addP.tangent = wiMath::CompressNormal(tan);

	//				float lbar = lenMod[0] + f*(lenMod[1]-lenMod[0]) + g*(lenMod[2]-lenMod[0]);
	//				addP.posLen.w = length*lbar + (float)(wiRandom::getRandom(0, 1000) - 500)*0.001f*length*lbar;
	//				addP.normalRand |= (uint8_t)wiRandom::getRandom(0, 256) << 24;
	//				points.push_back(addP);
	//			}

	//		}
	//	}
	//}

	//particleCount = points.size();

	//SAFE_DELETE(cb);
	//SAFE_DELETE(particleBuffer);
	//SAFE_DELETE(ib);
	//SAFE_DELETE(ib_transposed);

	//GPUBufferDesc bd;
	//ZeroMemory(&bd, sizeof(bd));
	//bd.Usage = USAGE_IMMUTABLE;
	//bd.ByteWidth = (UINT)(sizeof(Patch) * particleCount);
	//bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
	//bd.CPUAccessFlags = 0;
	//bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	//SubresourceData data = {};
	//data.pSysMem = points.data();
	//particleBuffer = new GPUBuffer;
	//wiRenderer::GetDevice()->CreateBuffer(&bd, &data, particleBuffer);


	//uint32_t* indices = new uint32_t[particleCount];
	//for (size_t i = 0; i < points.size(); ++i)
	//{
	//	indices[i] = (uint32_t)i;
	//}
	//data.pSysMem = indices;

	//bd.Usage = USAGE_DEFAULT;
	//bd.ByteWidth = (UINT)(sizeof(uint32_t) * particleCount);
	//bd.BindFlags = BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS;
	//bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	//ib = new GPUBuffer;
	//wiRenderer::GetDevice()->CreateBuffer(&bd, &data, ib);
	//ib_transposed = new GPUBuffer;
	//wiRenderer::GetDevice()->CreateBuffer(&bd, &data, ib_transposed);

	//SAFE_DELETE_ARRAY(indices);


	//IndirectDrawArgsIndexedInstanced args;
	//args.BaseVertexLocation = 0;
	//args.IndexCountPerInstance = (UINT)points.size();
	//args.InstanceCount = 1;
	//args.StartIndexLocation = 0;
	//args.StartInstanceLocation = 0;
	//data.pSysMem = &args;

	//bd.ByteWidth = (UINT)(sizeof(IndirectDrawArgsIndexedInstanced));
	//bd.MiscFlags = RESOURCE_MISC_DRAWINDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	//bd.BindFlags = BIND_UNORDERED_ACCESS;
	//drawargs = new GPUBuffer;
	//wiRenderer::GetDevice()->CreateBuffer(&bd, &data, drawargs);




	//ZeroMemory(&bd, sizeof(bd));
	//bd.Usage = USAGE_DYNAMIC;
	//bd.ByteWidth = sizeof(ConstantBuffer);
	//bd.BindFlags = BIND_CONSTANT_BUFFER;
	//bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	//cb = new GPUBuffer;
	//wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, cb);

}

void wiHairParticle::ComputeCulling(CameraComponent* camera, GRAPHICSTHREAD threadID)
{
	//GraphicsDevice* device = wiRenderer::GetDevice();
	//device->EventBegin("HairParticle - Culling", threadID);

	//XMMATRIX inverseMat = XMLoadFloat4x4(&OriginalMatrix_Inverse);
	//XMMATRIX renderMatrix = inverseMat * object->getMatrix();

	//ConstantBuffer gcb;
	//gcb.mWorld = XMMatrixTranspose(renderMatrix);
	//gcb.color = material->baseColor;
	//gcb.LOD0 = (float)LOD[0];
	//gcb.LOD1 = (float)LOD[1];
	//gcb.LOD2 = (float)LOD[2];

	//device->UpdateBuffer(cb, &gcb, threadID);

	//// old solution removed, todo new solution

	//device->EventEnd(threadID);
}

void wiHairParticle::Draw(CameraComponent* camera, SHADERTYPE shaderType, bool transparent, GRAPHICSTHREAD threadID)
{
	//Texture2D* texture = material->texture;
	//texture = texture == nullptr ? wiTextureHelper::getInstance()->getWhite() : texture;

	//{
	//	GraphicsDevice* device = wiRenderer::GetDevice();
	//	device->EventBegin("HairParticle - Draw", threadID);

	//	if (wiRenderer::IsWireRender())
	//	{
	//		if (transparent || shaderType == SHADERTYPE_DEPTHONLY)
	//		{
	//			return;
	//		}
	//		device->BindGraphicsPSO(&PSO_wire, threadID);
	//		device->BindResource(VS, wiTextureHelper::getInstance()->getWhite(), TEXSLOT_ONDEMAND0, threadID);
	//		device->BindConstantBuffer(PS, cb, CB_GETBINDSLOT(ConstantBuffer), threadID);
	//	}
	//	else
	//	{
	//		device->BindGraphicsPSO(&PSO[shaderType][transparent], threadID);

	//		if (texture)
	//		{
	//			device->BindResource(PS, texture, TEXSLOT_ONDEMAND0, threadID);
	//			device->BindResource(VS, texture, TEXSLOT_ONDEMAND0, threadID);
	//		}
	//	}

	//	device->BindConstantBuffer(VS, cb, CB_GETBINDSLOT(ConstantBuffer),threadID);

	//	device->BindResource(VS, particleBuffer, 0, threadID);

	//	device->Draw((int)particleCount * 12, 0, threadID);

	//	device->EventEnd(threadID);
	//}
}

}
