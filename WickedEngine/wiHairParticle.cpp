#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiLoader.h"
#include "wiMath.h"
#include "wiFrustum.h"
#include "wiRandom.h"
#include "ResourceMapping.h"
#include "wiArchive.h"
#include "ShaderInterop.h"

using namespace std;
using namespace wiGraphicsTypes;

VertexShader *wiHairParticle::vs = nullptr;
PixelShader *wiHairParticle::ps[];
ComputeShader *wiHairParticle::cs_RESET = nullptr;
ComputeShader *wiHairParticle::cs_CULLING = nullptr;
ComputeShader *wiHairParticle::cs_BITONICSORT = nullptr;
ComputeShader *wiHairParticle::cs_TRANSPOSE = nullptr;
GPUBuffer *wiHairParticle::cb_BITONIC = nullptr;
DepthStencilState *wiHairParticle::dss = nullptr;
RasterizerState *wiHairParticle::rs = nullptr, *wiHairParticle::ncrs = nullptr;
BlendState *wiHairParticle::bs = nullptr;
int wiHairParticle::LOD[3];

wiHairParticle::wiHairParticle()
{
	cb = nullptr;
	particleBuffer = nullptr;
	ib = nullptr;
	ib_transposed = nullptr;
	name = "";
	densityG = "";
	lenG = "";
	length = 0;
	count = 0;
	material = nullptr;
	object = nullptr;
	materialName = ""; 
	particleCount = 0;
}
wiHairParticle::wiHairParticle(const std::string& newName, float newLen, int newCount
						   , const std::string& newMat, Object* newObject, const std::string& densityGroup, const std::string& lengthGroup)
{
	cb = nullptr;
	particleBuffer = nullptr;
	ib = nullptr;
	ib_transposed = nullptr;
	drawargs = nullptr;
	name=newName;
	densityG=densityGroup;
	lenG=lengthGroup;
	length=newLen;
	count=newCount;
	material=nullptr;
	object = newObject;
	materialName = newMat;
	particleCount = 0;
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
		Generate();
	}
}


void wiHairParticle::CleanUp()
{
	SAFE_DELETE(cb);
	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(ib);
	SAFE_DELETE(drawargs);
}

void wiHairParticle::CleanUpStatic()
{
	SAFE_DELETE(vs);
	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_DELETE(ps[i]);
	}
	SAFE_DELETE(cs_RESET);
	SAFE_DELETE(cs_CULLING);
	SAFE_DELETE(cs_BITONICSORT);
	SAFE_DELETE(cs_TRANSPOSE);
	SAFE_DELETE(cb_BITONIC);
	SAFE_DELETE(dss);
	SAFE_DELETE(rs);
	SAFE_DELETE(ncrs);
	SAFE_DELETE(bs);
}
void wiHairParticle::LoadShaders()
{
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassVS.cso", wiResourceManager::VERTEXSHADER));
	if (vsinfo != nullptr) {
		vs = vsinfo->vertexShader;
	}

	for (int i = 0; i < SHADERTYPE_COUNT; ++i)
	{
		SAFE_INIT(ps[i]);
	}
	
	ps[SHADERTYPE_DEPTHONLY] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_alphatestonly.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_DEFERRED] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_deferred.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_FORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_forward_dirlight.cso", wiResourceManager::PIXELSHADER));
	ps[SHADERTYPE_TILEDFORWARD] = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "qGrassPS_tiledforward.cso", wiResourceManager::PIXELSHADER));

	cs_RESET = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassCulling_RESETCS.cso", wiResourceManager::COMPUTESHADER));
	cs_CULLING = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "grassCullingCS.cso", wiResourceManager::COMPUTESHADER));
	cs_BITONICSORT = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "bitonicSort_hairparticleCS.cso", wiResourceManager::COMPUTESHADER));
	cs_TRANSPOSE = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "matrixTransposeCS.cso", wiResourceManager::COMPUTESHADER));

}
void wiHairParticle::SetUpStatic()
{
	Settings(10,25,120);

	LoadShaders();


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
	bld.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bld.RenderTarget[0].BlendOpAlpha = BLEND_OP_MAX;
	bld.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bld.AlphaToCoverageEnable=false; // maybe for msaa
	bs = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bld,bs);



	GPUBufferDesc bd;
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.ByteWidth = sizeof(BitonicSortConstantBuffer);
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.Usage = USAGE_DYNAMIC;
	cb_BITONIC = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, cb_BITONIC);
}
void wiHairParticle::Settings(int l0,int l1,int l2)
{
	LOD[0]=l0;
	LOD[1]=l1;
	LOD[2]=l2;
}


void wiHairParticle::Generate()
{
	std::vector<Point> points;

	Mesh* mesh = object->mesh;

	XMMATRIX matr = object->getMatrix();

	int dVG = -1, lVG = -1;
	if (densityG.compare("")) {
		for (unsigned int i = 0; i < mesh->vertexGroups.size(); ++i)
			if (!mesh->vertexGroups[i].name.compare(densityG))
				dVG = i;
	}
	if (lenG.compare("")) {
		for (unsigned int i = 0; i < mesh->vertexGroups.size(); ++i)
			if (!mesh->vertexGroups[i].name.compare(lenG))
				lVG = i;
	}
	
	float avgPatchSize;
	if(dVG>=0)
		avgPatchSize = (float)count/((float)mesh->vertexGroups[dVG].vertices.size()/3.0f);
	else
		avgPatchSize = (float)count/((float)mesh->indices.size()/3.0f);

	if (mesh->indices.size() < 4)
		return;

	for (unsigned int i = 0; i<mesh->indices.size() - 3; i += 3)
	{

		unsigned int vi[]={mesh->indices[i],mesh->indices[i+1],mesh->indices[i+2]};
		float denMod[]={1,1,1},lenMod[]={1,1,1};
		if (dVG >= 0) {
			auto found = mesh->vertexGroups[dVG].vertices.find(vi[0]);
			if (found != mesh->vertexGroups[dVG].vertices.end())
				denMod[0] = found->second;
			else
				continue;

			found = mesh->vertexGroups[dVG].vertices.find(vi[1]);
			if (found != mesh->vertexGroups[dVG].vertices.end())
				denMod[1] = found->second;
			else
				continue;

			found = mesh->vertexGroups[dVG].vertices.find(vi[2]);
			if (found != mesh->vertexGroups[dVG].vertices.end())
				denMod[2] = found->second;
			else
				continue;
		}
		if (lVG >= 0) {
			auto found = mesh->vertexGroups[lVG].vertices.find(vi[0]);
			if (found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[0] = found->second;
			else
				continue;

			found = mesh->vertexGroups[lVG].vertices.find(vi[1]);
			if (found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[1] = found->second;
			else
				continue;

			found = mesh->vertexGroups[lVG].vertices.find(vi[2]);
			if (found != mesh->vertexGroups[lVG].vertices.end())
				lenMod[2] = found->second;
			else
				continue;
		}
		for (int m = 0; m < 3; ++m) {
			if (denMod[m] < 0) denMod[m] = 0;
			if (lenMod[m] < 0) lenMod[m] = 0;
		}

		Mesh::Vertex_FULL verts[] = {
			mesh->vertices_FULL[vi[0]],
			mesh->vertices_FULL[vi[1]],
			mesh->vertices_FULL[vi[2]],
		};

		if(
			(denMod[0]>FLT_EPSILON || denMod[1]>FLT_EPSILON || denMod[2]>FLT_EPSILON) &&
			(lenMod[0]>FLT_EPSILON || lenMod[1]>FLT_EPSILON || lenMod[2]>FLT_EPSILON)
		  )
		{

			float density = (float)(denMod[0]+denMod[1]+denMod[2])/3.0f*avgPatchSize;
			int rdense = (int)(( density - (int)density ) * 100);
			density += ((wiRandom::getRandom(0, 99)) <= rdense ? 1.0f : 0.0f);
			int PATCHSIZE = material->texture?(int)density:(int)density*10;
			  
			if(PATCHSIZE)
			{

				for(int p=0;p<PATCHSIZE;++p)
				{
					float f = wiRandom::getRandom(0, 1000) * 0.001f, g = wiRandom::getRandom(0, 1000) * 0.001f;
					if (f + g > 1)
					{
						f = 1 - f;
						g = 1 - g;
					}
					XMVECTOR pos[] = {
						XMVector3Transform(XMLoadFloat4(&verts[0].pos),matr)
						,	XMVector3Transform(XMLoadFloat4(&verts[1].pos),matr)
						,	XMVector3Transform(XMLoadFloat4(&verts[2].pos),matr)
					};
					XMVECTOR vbar=XMVectorBaryCentric(
							pos[0],pos[1],pos[2]
						,	f
						,	g
						);
					XMVECTOR nbar=XMVectorBaryCentric(
							XMLoadFloat4(&verts[0].nor)
						,	XMLoadFloat4(&verts[1].nor)
						,	XMLoadFloat4(&verts[2].nor)
						,	f
						,	g
						);
					int ti = wiRandom::getRandom(0, 2);
					XMVECTOR tangent = XMVector3Normalize(XMVectorSubtract(pos[ti], pos[(ti + 1) % 3]));
					
					Point addP;
					::XMStoreFloat4(&addP.posLen,vbar);

					XMFLOAT3 nor, tan;
					::XMStoreFloat3(&nor,XMVector3Normalize(nbar));
					::XMStoreFloat3(&tan,tangent);

					addP.normalRand = wiMath::CompressNormal(nor);
					addP.tangent = wiMath::CompressNormal(tan);

					float lbar = lenMod[0] + f*(lenMod[1]-lenMod[0]) + g*(lenMod[2]-lenMod[0]);
					addP.posLen.w = length*lbar + (float)(wiRandom::getRandom(0, 1000) - 500)*0.001f*length*lbar;
					addP.normalRand |= (uint8_t)wiRandom::getRandom(0, 256) << 24;
					points.push_back(addP);
				}

			}
		}
	}

	particleCount = points.size();

	SAFE_DELETE(cb);
	SAFE_DELETE(particleBuffer);
	SAFE_DELETE(ib);
	SAFE_DELETE(ib_transposed);

	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_IMMUTABLE;
	bd.ByteWidth = (UINT)(sizeof(Point) * particleCount);
	bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	SubresourceData data = {};
	data.pSysMem = points.data();
	particleBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, particleBuffer);


	uint32_t* indices = new uint32_t[particleCount];
	for (size_t i = 0; i < points.size(); ++i)
	{
		indices[i] = (uint32_t)i;
	}
	data.pSysMem = indices;

	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = (UINT)(sizeof(uint32_t) * particleCount);
	bd.BindFlags = BIND_INDEX_BUFFER | BIND_UNORDERED_ACCESS;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	ib = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, ib);
	ib_transposed = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, ib_transposed);

	SAFE_DELETE_ARRAY(indices);


	IndirectDrawArgsIndexedInstanced args;
	args.BaseVertexLocation = 0;
	args.IndexCountPerInstance = (UINT)points.size();
	args.InstanceCount = 1;
	args.StartIndexLocation = 0;
	args.StartInstanceLocation = 0;
	data.pSysMem = &args;

	bd.ByteWidth = (UINT)(sizeof(IndirectDrawArgsIndexedInstanced));
	bd.MiscFlags = RESOURCE_MISC_DRAWINDIRECT_ARGS | RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	bd.BindFlags = BIND_UNORDERED_ACCESS;
	drawargs = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &data, drawargs);




	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	cb = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, cb);

}

//#define ENABLE_CULLING
//#define ENABLE_SORTING
void wiHairParticle::ComputeCulling(Camera* camera, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("HairParticle - Culling", threadID);

	XMMATRIX inverseMat = XMLoadFloat4x4(&OriginalMatrix_Inverse);
	XMMATRIX renderMatrix = inverseMat * object->getMatrix();

	ConstantBuffer gcb;
	gcb.mWorld = XMMatrixTranspose(renderMatrix);
	gcb.color = material->baseColor;
	gcb.LOD0 = (float)LOD[0];
	gcb.LOD1 = (float)LOD[1];
	gcb.LOD2 = (float)LOD[2];

	device->UpdateBuffer(cb, &gcb, threadID);

#ifdef ENABLE_CULLING

	device->BindConstantBufferCS(cb, CB_GETBINDSLOT(ConstantBuffer), threadID);

	device->BindResourceCS(vb, 0, threadID);

	const GPUUnorderedResource* uavs[] = {
		static_cast<const GPUUnorderedResource*>(drawargs),
		static_cast<const GPUUnorderedResource*>(ib),
	};
	device->BindUnorderedAccessResourcesCS(uavs, 0, ARRAYSIZE(uavs), threadID);

	// First clear the drawarg buffer:
	device->BindCS(cs_RESET, threadID);
	device->Dispatch(1, 1, 1, threadID);

	// Then compute frustum culling:
	device->BindCS(cs_CULLING, threadID);
	device->Dispatch((UINT)ceilf(static_cast<float>(particleCount) / GRASS_CULLING_THREADCOUNT), 1, 1, threadID);

#ifdef ENABLE_SORTING
	device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);

	// Then sort the particles:
	{
		const UINT NUM_ELEMENTS = wiMath::GetNextPowerOfTwo((UINT)particleCount);
		const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
		const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

		device->BindConstantBufferCS(cb_BITONIC, 0, threadID);

		// Sort the data
		// First sort the rows for the levels <= to the block size
		for (UINT level = 2; level <= BITONIC_BLOCK_SIZE; level = level * 2)
		{
			BitonicSortConstantBuffer cb = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
			device->UpdateBuffer(cb_BITONIC, &cb, threadID);

			// Sort the row data
			device->BindUnorderedAccessResourceCS(ib, 0, threadID);
			device->BindCS(cs_BITONICSORT, threadID);
			device->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1, threadID);
		}

		// Then sort the rows and columns for the levels > than the block size
		// Transpose. Sort the Columns. Transpose. Sort the Rows.
		for (UINT level = (BITONIC_BLOCK_SIZE * 2); level <= NUM_ELEMENTS; level = level * 2)
		{
			{
				BitonicSortConstantBuffer cb = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
				device->UpdateBuffer(cb_BITONIC, &cb, threadID);
			}

			// Transpose the data from buffer 1 into buffer 2
			device->UnBindResources(0, 1, threadID);
			device->BindUnorderedAccessResourceCS(ib_transposed, 0, threadID);
			device->BindResourceCS(ib, 0, threadID);
			device->BindCS(cs_TRANSPOSE, threadID);
			device->Dispatch(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1, threadID);

			// Sort the transposed column data
			device->BindCS(cs_BITONICSORT, threadID);
			device->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1, threadID);

			{
				BitonicSortConstantBuffer cb = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
				device->UpdateBuffer(cb_BITONIC, &cb, threadID);
			}

			// Transpose the data from buffer 2 back into buffer 1
			device->UnBindResources(0, 1, threadID);
			device->BindUnorderedAccessResourceCS(ib, 0, threadID);
			device->BindResourceCS(ib_transposed, 0, threadID);
			device->BindCS(cs_TRANSPOSE, threadID);
			device->Dispatch(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1, threadID);

			// Sort the row data
			device->BindCS(cs_BITONICSORT, threadID);
			device->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1, threadID);
		}
	}
#endif // ENABLE_SORTING

	// Then reset state:
	device->BindCS(nullptr, threadID);
	device->UnBindUnorderedAccessResources(0, ARRAYSIZE(uavs), threadID);
	device->UnBindResources(0, 1, threadID);

#endif // ENABLE_CULLING

	device->EventEnd(threadID);
}

void wiHairParticle::Draw(Camera* camera, SHADERTYPE shaderType, GRAPHICSTHREAD threadID)
{
	Texture2D* texture = material->texture;
	PixelShader* _ps = ps[shaderType];
	if (_ps == nullptr)
		return;

	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("HairParticle - Draw", threadID);


		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
		device->BindVertexLayout(nullptr,threadID);
		device->BindPS(_ps,threadID);
		device->BindVS(vs,threadID);

		if(texture)
		{
			device->BindResourcePS(texture,TEXSLOT_ONDEMAND0,threadID);
			device->BindResourceVS(texture,TEXSLOT_ONDEMAND0,threadID);
		}

		device->BindRasterizerState(ncrs, threadID);

		device->BindConstantBufferVS(cb, CB_GETBINDSLOT(ConstantBuffer),threadID);

		device->BindResourceVS(particleBuffer, 0, threadID);

		device->Draw((int)particleCount * 12, threadID);

		//device->BindIndexBuffer(ib, INDEXBUFFER_FORMAT::INDEXFORMAT_32BIT, threadID);
		//device->DrawIndexedInstancedIndirect(drawargs, 0, threadID);

		device->BindGS(nullptr,threadID);

		device->EventEnd(threadID);
	}
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
