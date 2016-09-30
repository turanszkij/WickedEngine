#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiLoader.h"
#include "wiMath.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"

using namespace wiGraphicsTypes;

VertexLayout *wiHairParticle::il;
VertexShader *wiHairParticle::vs;
PixelShader *wiHairParticle::ps[],*wiHairParticle::qps[];
GeometryShader *wiHairParticle::gs[],*wiHairParticle::qgs[];
GPUBuffer *wiHairParticle::cbgs;
DepthStencilState *wiHairParticle::dss;
RasterizerState *wiHairParticle::rs, *wiHairParticle::ncrs;
BlendState *wiHairParticle::bs;
int wiHairParticle::LOD[3];

wiHairParticle::wiHairParticle()
{
	name = "";
	densityG = "";
	lenG = "";
	length = 0;
	count = 0;
	material = nullptr;
	object = nullptr;
	materialName = "";
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
	materialName = newMat;
	XMStoreFloat4x4(&OriginalMatrix_Inverse, XMMatrixInverse(nullptr, object->getMatrix()));
	for (MeshSubset& subset : object->mesh->subsets)
	{
		if (!newMat.compare(subset.material->name)) {
			material = subset.material;
			break;
		}
	}
	
	if (material)
	{
		SetUpPatches();
	}
}


void wiHairParticle::CleanUp(){
	for (unsigned int i = 0; i<patches.size(); ++i)
		patches[i]->CleanUp();
	patches.clear();
	SAFE_DELETE(vb[0]);
	SAFE_DELETE(vb[1]);
	SAFE_DELETE(vb[2]);
}

void wiHairParticle::CleanUpStatic(){
	SAFE_DELETE(il);
	SAFE_DELETE(vs);
	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_DELETE(ps[i]);
		SAFE_DELETE(qps[i]);
	}
	SAFE_DELETE(gs[0]);
	SAFE_DELETE(gs[1]);
	SAFE_DELETE(gs[2]);
	SAFE_DELETE(qgs[0]);
	SAFE_DELETE(qgs[1]);
	SAFE_DELETE(cbgs);
	SAFE_DELETE(dss);
	SAFE_DELETE(rs);
	SAFE_DELETE(ncrs);
	SAFE_DELETE(bs);
}
void wiHairParticle::LoadShaders()
{

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, FORMAT_R32G32B32A32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr) {
		vs = vsinfo->vertexShader;
		il = vsinfo->vertexLayout;
	}

	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_INIT(ps[i]);
		SAFE_INIT(qps[i]);
	}

	ps[SHADERTYPE_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassPS_deferred.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_TILEDFORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassPS_tiledforward.cso", wiResourceManager::PIXELSHADER));
	qps[SHADERTYPE_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_deferred.cso", wiResourceManager::PIXELSHADER));
	qps[SHADERTYPE_TILEDFORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_tiledforward.cso", wiResourceManager::PIXELSHADER));

	gs[0] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL0GS.cso", wiResourceManager::GEOMETRYSHADER));
	gs[1] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL1GS.cso", wiResourceManager::GEOMETRYSHADER));
	gs[2] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassL2GS.cso", wiResourceManager::GEOMETRYSHADER));

	qgs[0] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassLCloseGS.cso", wiResourceManager::GEOMETRYSHADER));
	qgs[1] = static_cast<GeometryShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassLDistGS.cso", wiResourceManager::GEOMETRYSHADER));

}
void wiHairParticle::SetUpStatic(){
	Settings(10,25,120);

	LoadShaders();

	
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	cbgs = new GPUBuffer;
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, cbgs );

	

	
	RasterizerStateDesc rsd;
	rsd.FillMode=FILL_SOLID;
	rsd.CullMode=CULL_BACK;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.ScissorEnable=false;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
	rs = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd,rs);

	rsd.FillMode=FILL_SOLID;
	rsd.CullMode=CULL_NONE;
	rsd.FrontCounterClockwise=true;
	rsd.DepthBias=0;
	rsd.DepthBiasClamp=0;
	rsd.SlopeScaledDepthBias=0;
	rsd.DepthClipEnable=true;
	rsd.ScissorEnable=false;
	rsd.MultisampleEnable=false;
	rsd.AntialiasedLineEnable=false;
	ncrs = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rsd,ncrs);

	
	DepthStencilStateDesc dsd;
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = COMPARISON_LESS;

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
	// Create the depth stencil state.
	dss = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, dss);

	
	BlendStateDesc bld;
	ZeroMemory(&bld, sizeof(bld));
	bld.RenderTarget[0].BlendEnable=false;
	bld.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bld.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bld.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bld.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bld.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	bld.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bld.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bld.AlphaToCoverageEnable=false;
	bs = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bld,bs);
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
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Point)*MAX_PARTICLES;
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	vb[0] = new GPUBuffer;
	vb[1] = new GPUBuffer;
	vb[2] = new GPUBuffer;
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, vb[0] );
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, vb[1] );
    wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, vb[2] );

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
void wiHairParticle::Draw(Camera* camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID)
{
	Texture2D* texture = material->texture;
	texture = nullptr;
	PixelShader* _ps = texture != nullptr ? qps[shaderType] : ps[shaderType];
	if (_ps == nullptr)
		return;

	XMMATRIX inverseMat = XMLoadFloat4x4(&OriginalMatrix_Inverse);
	XMMATRIX renderMatrix = inverseMat * object->getMatrix();
	XMMATRIX inverseRenderMat = XMMatrixInverse(nullptr, renderMatrix);

	Frustum frustum;

	// Culling camera needs to be transformed according to hair ps transform (inverse) 
	XMFLOAT4X4 cull_View;
	XMFLOAT3 eye;
	{
		XMMATRIX View;
		XMVECTOR At = XMVectorSet(0, 0, 1, 0), Up = XMVectorSet(0, 1, 0, 0), Eye;

		XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&camera->rotation));
		At = XMVector3Normalize(XMVector3Transform(XMVector3Transform(At, camRot), inverseRenderMat));
		Up = XMVector3Normalize(XMVector3Transform(XMVector3Transform(Up, camRot), inverseRenderMat));
		Eye = XMVector4Transform(camera->GetEye(), inverseRenderMat);

		View = XMMatrixLookToLH(Eye, At, Up);

		XMStoreFloat4x4(&cull_View, View);
		XMStoreFloat3(&eye, Eye);
	}

	frustum.ConstructFrustum((float)LOD[2], camera->Projection, cull_View);

		
	CulledList culledPatches;
	if(spTree)
		wiSPTree::getVisible(spTree->root,frustum,culledPatches);

	if(!culledPatches.empty())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin(L"HairParticle", threadID);


		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::POINTLIST,threadID);
		device->BindVertexLayout(il,threadID);
		device->BindPS(_ps,threadID);
		device->BindVS(vs,threadID);

		if(texture){
			device->BindResourcePS(texture,TEXSLOT_ONDEMAND0,threadID);
			device->BindResourceGS(texture,TEXSLOT_ONDEMAND0,threadID);

			device->BindBlendState(bs,threadID);
		}
		else
			device->BindRasterizerState(ncrs,threadID);


		ConstantBuffer gcb;
		gcb.mWorld = XMMatrixTranspose(renderMatrix);
		gcb.color=material->diffuseColor;
		gcb.drawdistance = (float)LOD[2];
		
		device->UpdateBuffer(cbgs,&gcb,threadID);
		device->BindConstantBufferGS(cbgs, CB_GETBINDSLOT(ConstantBuffer),threadID);

		device->BindDepthStencilState(dss,STENCILREF_DEFAULT,threadID);


		for(int i=0;i<3;++i){
			vector<Point> renderPoints;
			renderPoints.reserve(MAX_PARTICLES);

			if(texture){
				device->BindGS(i<2?qgs[0]:qgs[1],threadID);
				device->BindRasterizerState(i<2?ncrs:rs,threadID);
			}
			else
				device->BindGS(gs[i],threadID);
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

			device->UpdateBuffer(vb[i],renderPoints.data(),threadID,(int)(sizeof(Point)*renderPoints.size()));
			device->BindVertexBuffer(vb[i],0,sizeof(Point),threadID);
			device->Draw((int)renderPoints.size(),threadID);
		}

		device->BindGS(nullptr,threadID);

		device->EventEnd(threadID);
	}
}

wiHairParticle::Patch::Patch(){
	p.resize(0);
	min=XMFLOAT3(FLOAT32_MAX,FLOAT32_MAX,FLOAT32_MAX);
	max=XMFLOAT3(-FLOAT32_MAX,-FLOAT32_MAX,-FLOAT32_MAX);
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


void wiHairParticle::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> length;
		archive >> count;
		archive >> name;
		archive >> densityG;
		archive >> lenG;
		archive >> materialName;
		archive >> OriginalMatrix_Inverse;
	}
	else
	{
		archive << length;
		archive << count;
		archive << name;
		archive << densityG;
		archive << lenG;
		archive << materialName;
		archive << OriginalMatrix_Inverse;
	}
}
