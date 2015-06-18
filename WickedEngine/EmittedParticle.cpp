#include "EmittedParticle.h"

extern Camera* cam;

//ID3D11Buffer		*EmittedParticle::vertexBuffer;
ID3D11InputLayout   *EmittedParticle::vertexLayout;
ID3D11VertexShader  *EmittedParticle::vertexShader;
ID3D11PixelShader   *EmittedParticle::pixelShader,*EmittedParticle::simplestPS;
ID3D11GeometryShader*EmittedParticle::geometryShader;
ID3D11Buffer*           EmittedParticle::constantBuffer;
ID3D11BlendState*		EmittedParticle::blendStateAlpha,*EmittedParticle::blendStateAdd;
ID3D11SamplerState*			EmittedParticle::sampleState;
ID3D11RasterizerState*		EmittedParticle::rasterizerState,*EmittedParticle::wireFrameRS;
ID3D11DepthStencilState*	EmittedParticle::depthStencilState;
set<EmittedParticle*> EmittedParticle::systems;

EmittedParticle::EmittedParticle(std::string newName, std::string newMat, Object* newObject, float newSize, float newRandomFac, float newNormalFac
		,float newCount, float newLife, float newRandLife, float newScaleX, float newScaleY, float newRot){
	name=newName;
	object=newObject;
	for(Material*mat : object->mesh->materials)
		if(!mat->name.compare(newMat)){
			material=mat;
			break;
		}

	size=newSize;
	random_factor=newRandomFac;
	normal_factor=newNormalFac;

	count=newCount;
	points.resize(0);
	life=newLife;
	random_life=newRandLife;
	emit=0;
	
	scaleX=newScaleX;
	scaleY=newScaleY;
	rotation = newRot;

	bounding_box=new AABB();
	lastSquaredDistMulThousand=0;
	light=nullptr;
	if(material->blendFlag==BLENDMODE_ADDITIVE){
		light=new Light();
		light->color.x=material->diffuseColor.x;
		light->color.y=material->diffuseColor.y;
		light->color.z=material->diffuseColor.z;
		light->type=Light::POINT;
		light->name="particleSystemLight";
		//light->shadowMap.resize(1);
		//light->shadowMap[0].InitializeCube(Renderer::POINTLIGHTSHADOWRES,0,true);
	}

	LoadVertexBuffer();

	XMFLOAT4X4 transform = XMFLOAT4X4(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
	transform4 = transform;
	transform3 = XMFLOAT3X3(
		transform._11,transform._12,transform._13
		,transform._21,transform._22,transform._23
		,transform._31,transform._32,transform._33
		);

}
long EmittedParticle::getCount(){return points.size();}


int EmittedParticle::getRandomPointOnEmitter(){ return rand()%(object->mesh->indices.size()); }


void EmittedParticle::addPoint(const XMMATRIX& t4, const XMMATRIX& t3)
{
	vector<SkinnedVertex>& emitterVertexList = object->mesh->vertices;
	int gen[3];
	gen[0] = getRandomPointOnEmitter();
	switch(gen[0]%3)
	{
	case 0:
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	case 1:
		gen[0]=gen[0]-1;
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	case 2:
		gen[0]=gen[0]-2;
		gen[1]=gen[0]+1;
		gen[2]=gen[0]+2;
		break;
	default:
		break;
	}
	float f=rand()%1000 * 0.001f,g=rand()%1000 * 0.001f;
	if (f + g > 1)
	{
		f = 1 - f;
		g = 1 - g;
	}

	XMFLOAT3 pos;
	XMFLOAT3 vel;
	XMVECTOR& vbar=XMVectorBaryCentric(
			XMLoadFloat4(&emitterVertexList[object->mesh->indices[gen[0]]].pos)
		,	XMLoadFloat4(&emitterVertexList[object->mesh->indices[gen[1]]].pos)
		,	XMLoadFloat4(&emitterVertexList[object->mesh->indices[gen[2]]].pos)
		,	f
		,	g
		);
	XMVECTOR& nbar=XMVectorBaryCentric(
			XMLoadFloat3(&emitterVertexList[object->mesh->indices[gen[0]]].nor)
		,	XMLoadFloat3(&emitterVertexList[object->mesh->indices[gen[1]]].nor)
		,	XMLoadFloat3(&emitterVertexList[object->mesh->indices[gen[2]]].nor)
		,	f
		,	g
		);
	XMStoreFloat3( &pos, XMVector3Transform( vbar, t4 ) );
	XMStoreFloat3( &vel, XMVector3Normalize( XMVector3Transform( nbar, t3 ) ));
			
	float vrand = (normal_factor*getNewVelocityModifier())/60.0f;

	vel.x*=vrand;
	vel.y*=vrand;
	vel.z*=vrand;

		//pos.x+=getNewPositionModifier();
		//pos.y+=getNewPositionModifier();
		//pos.z+=getNewPositionModifier();


	points.push_back( Point( pos, XMFLOAT4(size,1,rand()%2,rand()%2), vel/*, XMFLOAT3(1,1,1)*/, getNewLifeSpan()
		,rotation*getNewRotationModifier(),scaleX,scaleY ) );
}
void EmittedParticle::Update(float gamespeed)
{
	systems.insert(this);


	XMFLOAT3 minP=XMFLOAT3(D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX)
		,maxP=XMFLOAT3(-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX);

	for(int i=0;i<points.size();++i){
		Point &point = points[i];

		point.pos.x += point.vel.x*gamespeed;
		point.pos.y += point.vel.y*gamespeed;
		point.pos.z += point.vel.z*gamespeed;
		point.rot += point.rotVel*gamespeed;
		

		/*if(point.maxLife-point.life<point.maxLife*0.1f)
			point.sizOpaMir.y-=0.05f*gamespeed;
		if(point.maxLife-point.life>point.maxLife*0.9f)
			point.sizOpaMir.y+=0.05f*gamespeed;
		if(point.sizOpaMir.y<=0) point.sizOpaMir.y=0;
		if(point.sizOpaMir.y>=1) point.sizOpaMir.y=1;*/

		point.life-=/*1.0f/60.0f**/gamespeed;
		point.life=WickedMath::Clamp(point.life,0,point.maxLife);

		float lifeLerp = point.life/point.maxLife;
		point.sizOpaMir.x=WickedMath::Lerp(point.sizBeginEnd[1],point.sizBeginEnd[0],lifeLerp);
		point.sizOpaMir.y=WickedMath::Lerp(1,0,lifeLerp);

		
		minP=WickedMath::Min(XMFLOAT3(point.pos.x-point.sizOpaMir.x,point.pos.y-point.sizOpaMir.x,point.pos.z-point.sizOpaMir.x),minP);
		maxP=WickedMath::Max(XMFLOAT3(point.pos.x+point.sizOpaMir.x,point.pos.y+point.sizOpaMir.x,point.pos.z+point.sizOpaMir.x),maxP);
	}
	bounding_box->create(minP,maxP);

	while(!points.empty() && points.front().life<=0)
		points.pop_front();


	XMFLOAT4X4& transform = object->world;
	transform4 = transform;
	transform3 = XMFLOAT3X3(
		transform._11,transform._12,transform._13
		,transform._21,transform._22,transform._23
		,transform._31,transform._32,transform._33
		);
	XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);
	
	emit += (float)count/60.0f*gamespeed;

	bool clearSpace=false;
	if(points.size()+emit>=MAX_PARTICLES)
		clearSpace=true;

	for(int i=0;i<(int)emit;++i)
	{
		if(clearSpace)
			points.pop_front();

		addPoint(t4,t3);
	}
	if((int)emit>0)
		emit=0;
	
	if(light!=nullptr){
		light->translation_rest=bounding_box->getCenter();
		light->enerDis=XMFLOAT4(5,bounding_box->getRadius()*3,0,0);
	}
	
	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//Point* vertexPtr;
	//Renderer::immediateContext->Map(vertexBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//vertexPtr = (Point*)mappedResource.pData;
	//memcpy(vertexPtr,renderPoints.data(),sizeof(Point)* renderPoints.size());
	//Renderer::immediateContext->Unmap(vertexBuffer,0);
}
void EmittedParticle::Burst(float num)
{
	XMMATRIX t4=XMLoadFloat4x4(&transform4), t3=XMLoadFloat3x3(&transform3);

	static float burst = 0;
	burst+=num;
	for(int i=0;i<(int)burst;++i)
		addPoint(t4,t3);
	burst-=(int)burst;
}


void EmittedParticle::Draw(const XMVECTOR eye, const XMMATRIX& newView, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG)
{
	if(!points.empty()){
		
		Frustum frustum = Frustum();
		XMFLOAT4X4 proj,view;
		XMStoreFloat4x4( &proj,cam->Projection );
		XMStoreFloat4x4( &view,newView );
		frustum.ConstructFrustum(cam->zFarP,proj,view);

		if(frustum.CheckBox(bounding_box->corners)){
			
			vector<Point> renderPoints=vector<Point>(points.begin(),points.end());
			Renderer::UpdateBuffer(vertexBuffer,renderPoints.data(),context,sizeof(Point)* renderPoints.size());

			bool additive = (material->blendFlag==BLENDMODE_ADDITIVE || material->premultipliedTexture);

			//context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
			//context->IASetInputLayout( vertexLayout );
			//context->VSSetShader( vertexShader, NULL, 0 );
			//context->GSSetShader( geometryShader, NULL, 0 );
			//context->PSSetShader( wireRender?simplestPS:pixelShader, NULL, 0 );
			Renderer::BindPrimitiveTopology(Renderer::PRIMITIVETOPOLOGY::POINTLIST,context);
			Renderer::BindVertexLayout(vertexLayout,context);
			Renderer::BindPS(wireRender?simplestPS:pixelShader,context);
			Renderer::BindVS(vertexShader,context);
			Renderer::BindGS(geometryShader,context);
		
			Renderer::BindTexturePS(depth,1,context);

			ConstantBuffer cb;
			cb.mView = XMMatrixTranspose(newView);
			cb.mProjection = XMMatrixTranspose( cam->Projection );
			cb.mCamPos = eye;
			cb.mAdd.x = additive;
			cb.mAdd.y = (FLAG==DRAW_DARK?true:false);

		
			//context->UpdateSubresource( constantBuffer, 0, NULL, &cb, 0, 0 );

			Renderer::UpdateBuffer(constantBuffer,&cb,context);
			//D3D11_MAPPED_SUBRESOURCE mappedResource;
			//ConstantBuffer* dataPtr;
			//context->Map(constantBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
			//dataPtr = (ConstantBuffer*)mappedResource.pData;
			//memcpy(dataPtr,&cb,sizeof(ConstantBuffer));
			//context->Unmap(constantBuffer,0);
	
			//context->RSSetState(wireRender?wireFrameRS:rasterizerState);
			//context->OMSetDepthStencilState(depthStencilState, 1);
			Renderer::BindRasterizerState(wireRender?wireFrameRS:rasterizerState,context);
			Renderer::BindDepthStencilState(depthStencilState,1,context);
	
			//float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			//UINT sampleMask   = 0xffffffff;
			//context->OMSetBlendState((additive?blendStateAdd:blendStateAlpha), blendFactor, sampleMask);
			Renderer::BindBlendState((additive?blendStateAdd:blendStateAlpha),context);

			//context->GSSetConstantBuffers( 0, 1, &constantBuffer );
			//context->PSSetSamplers(0, 1, &sampleState);
			Renderer::BindConstantBufferGS(constantBuffer,0,context);
			Renderer::BindSamplerPS(sampleState,0,context);

		
			//context->UpdateSubresource( vertexBuffer, 0, 0, points.data(), 0, 0 );


			//UINT stride = sizeof( Point );
			//UINT offset = 0;
			//context->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
			Renderer::BindVertexBuffer(vertexBuffer,0,sizeof(Point),context);

			if(!wireRender && material->texture) Renderer::BindTexturePS(material->texture,0,context);
			//context->Draw( renderPoints.size(),0);
			Renderer::Draw(renderPoints.size(),context);


			//context->GSSetShader(0,0,0);
			Renderer::BindGS(nullptr,context);
			//context->GSSetConstantBuffers(0,0,0);
			Renderer::BindConstantBufferGS(nullptr,0,context);
		}
	}
}
void EmittedParticle::DrawPremul(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG){
	if(material->premultipliedTexture)
		Draw(eye,view,context,depth,FLAG);
}
void EmittedParticle::DrawNonPremul(const XMVECTOR eye, const XMMATRIX& view, ID3D11DeviceContext *context, ID3D11ShaderResourceView* depth, int FLAG){
	if(!material->premultipliedTexture)
		Draw(eye,view,context,depth,FLAG);
}


void EmittedParticle::CleanUp()
{

	points.clear();

	if(vertexBuffer) vertexBuffer->Release(); vertexBuffer=NULL;

	systems.erase(this);

	delete bounding_box;
	bounding_box=nullptr;

	//delete(this);
}

void* EmittedParticle::operator new(size_t size)
{
	void* result = _aligned_malloc(size,16);
	return result;
}
void EmittedParticle::operator delete(void* p)
{
	if(p) _aligned_free(p);
}





void EmittedParticle::LoadShaders()
{
    ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/pointspriteVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load pointspriteVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	
	


    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );
	
    // Create the input layout
	Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &vertexLayout );

	if(pVSBlob){ pVSBlob->Release();pVSBlob=NULL; }

    

	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/pointspritePS.cso", &pPSBlob))){MessageBox(0,L"Failed To load pointspritePS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );

	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }

	if(FAILED(D3DReadFileToBlob(L"shaders/pointspritePS_simplest.cso", &pPSBlob))){MessageBox(0,L"Failed To load pointspritePS_simplest.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &simplestPS );

	if(pPSBlob){ pPSBlob->Release();pPSBlob=NULL; }


	ID3DBlob* pGSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/pointspriteGS.cso", &pGSBlob))){MessageBox(0,L"Failed To load pointspriteGS.cso",0,0);}
	Renderer::graphicsDevice->CreateGeometryShader( pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &geometryShader );

	if(pGSBlob){ pGSBlob->Release();pGSBlob=NULL; }


}
void EmittedParticle::SetUpCB()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
}
void EmittedParticle::SetUpStates()
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Renderer::graphicsDevice->CreateSamplerState(&samplerDesc, &sampleState);



	
	D3D11_RASTERIZER_DESC rs;
	rs.FillMode=D3D11_FILL_SOLID;
	rs.CullMode=D3D11_CULL_BACK;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	Renderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);

	
	rs.FillMode=D3D11_FILL_WIREFRAME;
	rs.CullMode=D3D11_CULL_NONE;
	rs.FrontCounterClockwise=true;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=false;
	rs.ScissorEnable=false;
	rs.MultisampleEnable=false;
	rs.AntialiasedLineEnable=false;
	Renderer::graphicsDevice->CreateRasterizerState(&rs,&wireFrameRS);




	
	D3D11_DEPTH_STENCIL_DESC dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	Renderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);


	
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendStateAlpha);

	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable=true;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	bd.IndependentBlendEnable=true;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendStateAdd);
}
void EmittedParticle::LoadVertexBuffer()
{
	vertexBuffer=NULL;

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof( Point ) * MAX_PARTICLES;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &vertexBuffer );
}
void EmittedParticle::SetUpStatic()
{
	vertexShader = NULL;
	pixelShader = NULL;
	geometryShader = NULL;
	vertexLayout = NULL;
	constantBuffer = NULL;
	sampleState = NULL;
	rasterizerState = NULL;
	blendStateAdd = NULL;
	blendStateAlpha = NULL;
	depthStencilState = NULL;

	LoadShaders();
	SetUpCB();
	SetUpStates();

	systems.clear();
}
void EmittedParticle::CleanUpStatic()
{
	//if(vertexBuffer) vertexBuffer->Release(); vertexBuffer=NULL;
	if(vertexShader) vertexShader->Release(); vertexShader = NULL;
	if(pixelShader) pixelShader->Release(); pixelShader = NULL;
	if(simplestPS) simplestPS->Release(); simplestPS = NULL;
	if(geometryShader) geometryShader->Release(); geometryShader = NULL;
	if(vertexLayout) vertexLayout->Release(); vertexLayout = NULL;

	if(constantBuffer) constantBuffer->Release(); constantBuffer = NULL;

	if(sampleState) sampleState->Release(); sampleState = NULL;
	if(rasterizerState) rasterizerState->Release(); rasterizerState = NULL;
	if(wireFrameRS) wireFrameRS->Release(); wireFrameRS = NULL;
	if(blendStateAdd) blendStateAdd->Release(); blendStateAdd = NULL;
	if(blendStateAlpha) blendStateAlpha->Release(); blendStateAlpha = NULL;
	if(depthStencilState) depthStencilState->Release(); depthStencilState = NULL;
}
long EmittedParticle::getNumParticles()
{
	long retval=0;
	for(EmittedParticle* e:systems)
		if(e)
			retval+=e->getCount();
	return retval;
}