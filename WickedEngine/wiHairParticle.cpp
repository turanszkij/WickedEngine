#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiLoader.h"
#include "wiMath.h"
#include "wiFrustum.h"
#include "wiRandom.h"

VertexLayout wiHairParticle::il;
VertexShader wiHairParticle::vs;
PixelShader wiHairParticle::ps,wiHairParticle::qps;
GeometryShader wiHairParticle::gs[],wiHairParticle::qgs[];
BufferResource wiHairParticle::cbgs;
DepthStencilState wiHairParticle::dss;
RasterizerState wiHairParticle::rs, wiHairParticle::ncrs;
Sampler wiHairParticle::ss;
BlendState wiHairParticle::bs;
int wiHairParticle::LOD[3];

wiHairParticle::wiHairParticle()
{
}
wiHairParticle::wiHairParticle(const string& newName, float newLen, int newCount
						   , const string& newMat, Object* newObject, const string& densityGroup, const string& lengthGroup)
{
	name=newName;
	densityG=densityGroup;
	lenG=lengthGroup;
	length=newLen;
	count=newCount;
	material=nullptr;
	object = newObject;
	XMStoreFloat4x4(&OriginalMatrix_Inverse, XMMatrixInverse(nullptr, object->getMatrix()));
	for(Material* m : object->mesh->materials)
		if(!newMat.compare(m->name)){
			material = m;
			break;
		}
	
	if(material)
		SetUpPatches();
}


void wiHairParticle::CleanUp(){
	for (unsigned int i = 0; i<patches.size(); ++i)
		patches[i]->CleanUp();
	patches.clear();
	wiRenderer::SafeRelease(vb[0]);
	wiRenderer::SafeRelease(vb[1]);
	wiRenderer::SafeRelease(vb[2]);
}

void wiHairParticle::CleanUpStatic(){
	if(il) il->Release(); il=NULL;
	if(vs) vs->Release(); vs=NULL;
	for(int i=0;i<3;++i){
		if(gs[i]) gs[i]->Release(); gs[i]=NULL;}
	for(int i=0;i<2;++i){
		if(qgs[i]) qgs[i]->Release(); qgs[i]=NULL;}
	if(ps) ps->Release(); ps=NULL;
	if(qps) qps->Release(); qps=NULL;
	if(cbgs) cbgs->Release(); cbgs=NULL;
	if(ss) ss->Release(); ss=NULL;
	if(bs) bs->Release(); bs=NULL;
	if(rs) rs->Release(); rs=NULL;
	if(ncrs) ncrs->Release(); ncrs=NULL;
	
}
void wiHairParticle::LoadShaders()
{

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr) {
		vs = vsinfo->vertexShader;
		il = vsinfo->vertexLayout;
	}



	ps = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassPS.cso", wiResourceManager::PIXELSHADER));
	qps = static_cast<PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS.cso", wiResourceManager::PIXELSHADER));

	gs[0] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL0GS.cso", wiResourceManager::GEOMETRYSHADER));
	gs[1] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL1GS.cso", wiResourceManager::GEOMETRYSHADER));
	gs[2] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL2GS.cso", wiResourceManager::GEOMETRYSHADER));

	qgs[0] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassLCloseGS.cso", wiResourceManager::GEOMETRYSHADER));
	qgs[1] = static_cast<GeometryShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassLDistGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiHairParticle::SetUpStatic(){
	Settings(10,25,120);

	LoadShaders();

	
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(CBGS);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &cbgs );
	

	

	
	D3D11_RASTERIZER_DESC rsd;
	rsd.FillMode=D3D11_FILL_SOLID;
	rsd.CullMode=D3D11_CULL_BACK;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.ScissorEnable=false;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
	wiRenderer::graphicsDevice->CreateRasterizerState(&rsd,&rs);
	rsd.FillMode=D3D11_FILL_SOLID;
	rsd.CullMode=D3D11_CULL_NONE;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.ScissorEnable=false;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
	wiRenderer::graphicsDevice->CreateRasterizerState(&rsd,&ncrs);

	
	D3D11_DEPTH_STENCIL_DESC dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Create the depth stencil state.
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &dss);

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	wiRenderer::graphicsDevice->CreateSamplerState(&samplerDesc, &ss);

	
	D3D11_BLEND_DESC bld;
	ZeroMemory(&bld, sizeof(bld));
	bld.RenderTarget[0].BlendEnable=false;
	bld.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bld.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bld.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bld.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bld.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bld.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bld.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bld.AlphaToCoverageEnable=false;
	wiRenderer::graphicsDevice->CreateBlendState(&bld,&bs);
}
void wiHairParticle::Settings(int l0,int l1,int l2)
{
	LOD[0]=l0;
	LOD[1]=l1;
	LOD[2]=l2;
}

struct PatchHolder:public Cullable
{
	wiHairParticle::Patch* patch;
	PatchHolder(){}
	PatchHolder(wiHairParticle::Patch* p){patch=p;}
};

void wiHairParticle::SetUpPatches()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Point)*MAX_PARTICLES;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &vb[0] );
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &vb[1] );
    wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &vb[2] );

	const int triangleperpatch = 4;
	int currentTris=0;

	vector<PatchHolder*> pholder;
	patches.push_back(new Patch());
	pholder.push_back(new PatchHolder(patches.back()));

	Mesh* mesh = object->mesh;

	XMMATRIX matr = object->getMatrix();

	int dVG=-1,lVG=-1;
	if(densityG.compare("")){
		for (unsigned int i = 0; i<mesh->vertexGroups.size(); ++i)
			if(!mesh->vertexGroups[i].name.compare(densityG))
				dVG=i;
	}
	if(lenG.compare("")){
		for (unsigned int i = 0; i<mesh->vertexGroups.size(); ++i)
			if(!mesh->vertexGroups[i].name.compare(lenG))
				lVG=i;
	}
	
	float avgPatchSize;
	if(dVG>=0)
		avgPatchSize = (float)count/((float)mesh->vertexGroups[dVG].vertices.size()/3.0f);
	else
		avgPatchSize = (float)count/((float)mesh->indices.size()/3.0f);

	if (mesh->indices.size() < 4)
		return;

	for (unsigned int i = 0; i<mesh->indices.size() - 3; i += 3){

		unsigned int vi[]={mesh->indices[i],mesh->indices[i+1],mesh->indices[i+2]};
		float denMod[]={1,1,1},lenMod[]={1,1,1};
		if(dVG>=0){
			auto found = mesh->vertexGroups[dVG].vertices.find(vi[0]);
			if(found!=mesh->vertexGroups[dVG].vertices.end())
				denMod[0]=found->second;
			else
				continue;

			found = mesh->vertexGroups[dVG].vertices.find(vi[1]);
			if(found!=mesh->vertexGroups[dVG].vertices.end())
				denMod[1]=found->second;
			else
				continue;

			found = mesh->vertexGroups[dVG].vertices.find(vi[2]);
			if(found!=mesh->vertexGroups[dVG].vertices.end())
				denMod[2]=found->second;
			else
				continue;
		}
		if(lVG>=0){
			auto found = mesh->vertexGroups[lVG].vertices.find(vi[0]);
			if(found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[0]=found->second;
			else
				continue;

			found = mesh->vertexGroups[lVG].vertices.find(vi[1]);
			if(found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[1]=found->second;
			else
				continue;

			found = mesh->vertexGroups[lVG].vertices.find(vi[2]);
			if(found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[2]=found->second;
			else
				continue;
		}
		for(int m=0;m<3;++m){
			if(denMod[m]<0) denMod[m]=0;
			if(lenMod[m]<0) lenMod[m]=0;
		}

		SkinnedVertex* v[] = {
			&mesh->vertices[vi[0]],
			&mesh->vertices[vi[1]],
			&mesh->vertices[vi[2]],
		};
		if(
			(denMod[0]>FLT_EPSILON || denMod[1]>FLT_EPSILON || denMod[2]>FLT_EPSILON) &&
			(lenMod[0]>FLT_EPSILON || lenMod[1]>FLT_EPSILON || lenMod[2]>FLT_EPSILON)
		  )
		{

			float density = (float)(denMod[0]+denMod[1]+denMod[2])/3.0f*avgPatchSize;
			int rdense = (int)(( density - (int)density ) * 100);
			density += (wiRandom::getRandom(0, 99)) <= rdense;
			int PATCHSIZE = material->texture?(int)density:(int)density*10;
			  
			if(PATCHSIZE){

				for(int p=0;p<PATCHSIZE;++p){
					float f = wiRandom::getRandom(0, 1000) * 0.001f, g = wiRandom::getRandom(0, 1000) * 0.001f;
					if (f + g > 1)
					{
						f = 1 - f;
						g = 1 - g;
					}
					XMVECTOR pos[] = {
						XMVector3Transform(XMLoadFloat4(&v[0]->pos),matr)
						,	XMVector3Transform(XMLoadFloat4(&v[1]->pos),matr)
						,	XMVector3Transform(XMLoadFloat4(&v[2]->pos),matr)
					};
					XMVECTOR vbar=XMVectorBaryCentric(
							pos[0],pos[1],pos[2]
						,	f
						,	g
						);
					XMVECTOR nbar=XMVectorBaryCentric(
							XMLoadFloat4(&v[0]->nor)
						,	XMLoadFloat4(&v[1]->nor)
						,	XMLoadFloat4(&v[2]->nor)
						,	f
						,	g
						);
					int ti = wiRandom::getRandom(0, 2);
					XMVECTOR tangent = XMVector3Normalize( XMVectorSubtract(pos[ti],pos[(ti+1)%3]) );
					
					Point addP;
					::XMStoreFloat4(&addP.posRand,vbar);
					::XMStoreFloat4(&addP.normalLen,XMVector3Normalize(nbar));
					::XMStoreFloat4(&addP.tangent,tangent);

					float lbar = lenMod[0] + f*(lenMod[1]-lenMod[0]) + g*(lenMod[2]-lenMod[0]);
					addP.normalLen.w = length*lbar + (float)(wiRandom::getRandom(0, 1000) - 500)*0.001f*length*lbar;
					addP.posRand.w = (float)wiRandom::getRandom(0, 1000);
					patches.back()->add(addP);
				
					XMFLOAT3 posN = XMFLOAT3(addP.posRand.x,addP.posRand.y,addP.posRand.z);
					patches.back()->min=wiMath::Min(patches.back()->min,posN);
					patches.back()->max=wiMath::Max(patches.back()->max,posN);
				}

				if(currentTris>=triangleperpatch){
					pholder.back()->bounds.create(patches.back()->min,patches.back()->max);
					currentTris=0;

					patches.push_back(new Patch());
					pholder.push_back(new PatchHolder(patches.back()));
				}
				else
					currentTris++;

			}
		}
	}

	GenerateSPTree(spTree,vector<Cullable*>(pholder.begin(),pholder.end()),SPTREE_GENERATE_OCTREE);
	return;
}
void wiHairParticle::Draw(Camera* camera, ID3D11DeviceContext *context)
{
	XMMATRIX inverseMat = XMLoadFloat4x4(&OriginalMatrix_Inverse);
	XMMATRIX renderMatrix = inverseMat * object->getMatrix();
	XMMATRIX inverseRenderMat = XMMatrixInverse(nullptr, renderMatrix);

	static thread_local Frustum frustum = Frustum();

	// Culling camera needs to be transformed according to hair ps transform (inverse) 
	XMFLOAT4X4 cull_View;
	XMFLOAT3 eye;
	{
		XMMATRIX View;
		XMVECTOR At = XMVectorSet(0, 0, 1, 0), Up = XMVectorSet(0, 1, 0, 0), Eye;

		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation));
		At = XMVector3Normalize(XMVector3Transform(XMVector3Transform(At, camRot), inverseRenderMat));
		Up = XMVector3Normalize(XMVector3Transform(XMVector3Transform(Up, camRot), inverseRenderMat));
		Eye = XMVector3Transform(camera->GetEye(), inverseRenderMat);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&cull_View, View);
		XMStoreFloat3(&eye, Eye);
	}

	frustum.ConstructFrustum((float)LOD[2], camera->Projection, cull_View);

		
	CulledList culledPatches;
	if(spTree)
		wiSPTree::getVisible(spTree->root,frustum,culledPatches,SP_TREE_STRICT_CULL);

	if(!culledPatches.empty())
	{
		TextureView texture = material->texture;

		wiRenderer::BindPrimitiveTopology(PRIMITIVETOPOLOGY::POINTLIST,context);
		wiRenderer::BindVertexLayout(il,context);
		wiRenderer::BindPS(texture?qps:ps,context);
		wiRenderer::BindVS(vs,context);

		if(texture){
			wiRenderer::BindTexturePS(texture,0,context);
			wiRenderer::BindSamplerPS(ss,0,context);
			wiRenderer::BindTextureGS(texture,0,context);

			wiRenderer::BindBlendState(bs,context);
		}
		else
			wiRenderer::BindRasterizerState(ncrs,context);


		static thread_local CBGS* gcb = new CBGS;
		(*gcb).mWorld = XMMatrixTranspose(renderMatrix);
		(*gcb).mView = XMMatrixTranspose(camera->GetView());
		(*gcb).mProj = XMMatrixTranspose(camera->GetProjection());
		(*gcb).mPrevViewProjection = XMMatrixTranspose(wiRenderer::prevFrameCam->GetViewProjection());
		(*gcb).colTime=XMFLOAT4(material->diffuseColor.x,material->diffuseColor.y,material->diffuseColor.z, wiRenderer::GetScene().wind.time);
		(*gcb).eye = eye;
		(*gcb).drawdistance = (float)LOD[2];
		(*gcb).wind=wiRenderer::GetScene().wind.direction;
		(*gcb).windRandomness=wiRenderer::GetScene().wind.randomness;
		(*gcb).windWaveSize=wiRenderer::GetScene().wind.waveSize;
		
		wiRenderer::UpdateBuffer(cbgs,gcb,context);

		wiRenderer::BindDepthStencilState(dss,STENCILREF_DEFAULT,context);
		wiRenderer::BindConstantBufferGS(cbgs,0,context);


		for(int i=0;i<3;++i){
			vector<Point> renderPoints;
			renderPoints.reserve(MAX_PARTICLES);

			if(texture){
				wiRenderer::BindGS(i<2?qgs[0]:qgs[1],context);
				wiRenderer::BindRasterizerState(i<2?ncrs:rs,context);
			}
			else
				wiRenderer::BindGS(gs[i],context);
			CulledList::iterator iter = culledPatches.begin();
			while(iter != culledPatches.end()){
				Cullable* culled = *iter;
				Patch* patch = ((PatchHolder*)culled)->patch;

				float dis = wiMath::Distance(eye,((PatchHolder*)culled)->bounds.getCenter());

				if(dis<LOD[i]){
					culledPatches.erase(iter++);
					culled=NULL;
					renderPoints.insert(renderPoints.end(),patch->p.begin(),patch->p.end());
				}
				else
					++iter;
			}

			wiRenderer::UpdateBuffer(vb[i],renderPoints.data(),context,sizeof(Point)*renderPoints.size());
			wiRenderer::BindVertexBuffer(vb[i],0,sizeof(Point),context);
			wiRenderer::Draw(renderPoints.size(),context);
		}

		wiRenderer::BindGS(nullptr,context);
		wiRenderer::BindConstantBufferGS(nullptr,0,context);
	}
}

wiHairParticle::Patch::Patch(){
	p.resize(0);
	min=XMFLOAT3(D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX);
	max=XMFLOAT3(-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX);
	//vb=NULL;
}
void wiHairParticle::Patch::add(const Point& pp){
	p.push_back(pp);
}
void wiHairParticle::Patch::CleanUp(){
	p.clear();
	//if(vb)
	//	vb->Release();
	//vb=NULL;
}
