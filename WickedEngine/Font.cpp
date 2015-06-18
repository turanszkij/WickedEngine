#include "Font.h"

extern Camera* cam;


ID3D11Buffer		*Font::vertexBuffer,*Font::indexBuffer;
ID3D11InputLayout   *Font::vertexLayout;
ID3D11VertexShader  *Font::vertexShader;
ID3D11PixelShader   *Font::pixelShader;
ID3D11BlendState*		Font::blendState;
ID3D11Buffer*           Font::constantBuffer;
ID3D11SamplerState*			Font::sampleState;
ID3D11RasterizerState*		Font::rasterizerState;
ID3D11DepthStencilState*	Font::depthStencilState;
int Font::RENDERWIDTH,Font::RENDERHEIGHT;
mutex Font::MUTEX;
UINT Font::textlen;
SHORT Font::line,Font::pos;
BOOL Font::toDraw;
DWORD Font::counter;
vector<Font::Vertex> Font::vertexList;
vector<Font::FontStyle> Font::fontStyles;

void Font::Initialize()
{
	line=pos=counter=0;
	toDraw=TRUE;
	textlen=0;
	line=pos=0;
	RENDERWIDTH=1280;
	RENDERHEIGHT=720;

	
	indexBuffer=NULL;
	vertexBuffer=NULL;
	vertexLayout=NULL;
	vertexShader=NULL;
	pixelShader=NULL;
	blendState=NULL;
	constantBuffer=NULL;
	sampleState=NULL;
	rasterizerState=NULL;
	depthStencilState=NULL;
}

void Font::SetUpStates()
{
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
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
	rs.FrontCounterClockwise=TRUE;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=FALSE;
	rs.ScissorEnable=FALSE;
	rs.MultisampleEnable=FALSE;
	rs.AntialiasedLineEnable=FALSE;
	Renderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
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
	bd.RenderTarget[0].BlendEnable=TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	Renderer::graphicsDevice->CreateBlendState(&bd,&blendState);
}
void Font::SetUpCB()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
}
void Font::LoadShaders()
{
    ID3DBlob* pVSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/fontVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load fontVS.cso",0,0);}
	Renderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	
	


    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );
	
    // Create the input layout
	Renderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &vertexLayout );
	pVSBlob->Release();

    

	
	ID3DBlob* pPSBlob = NULL;

	if(FAILED(D3DReadFileToBlob(L"shaders/fontPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load fontPS.cso",0,0);}
	Renderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	pPSBlob->Release();
}
void Font::SetUpStaticComponents()
{
	SetUpStates();
	SetUpCB();
	LoadShaders();
	LoadVertexBuffer();
	LoadIndices();
}
void Font::CleanUpStatic()
{
	for(int i=0;i<fontStyles.size();++i) fontStyles[i].CleanUp();
	fontStyles.clear();

	vertexList.clear();
	if(vertexBuffer) vertexBuffer->Release();	vertexBuffer=NULL;
	if(indexBuffer) indexBuffer->Release();		indexBuffer=NULL;
	if(vertexShader) vertexShader->Release();	vertexShader=NULL;
	if(pixelShader) pixelShader->Release();		pixelShader=NULL;
	if(vertexLayout) vertexLayout->Release();	vertexLayout=NULL;
	if(constantBuffer) constantBuffer->Release();	constantBuffer=NULL;
	if(sampleState) sampleState->Release();		sampleState=NULL;
	if(rasterizerState) rasterizerState->Release();	rasterizerState=NULL;
	if(blendState) blendState->Release();		blendState=NULL;
	if(depthStencilState) depthStencilState->Release();	depthStencilState=NULL;
}


void Font::ModifyGeo(const wchar_t* text, XMFLOAT2 sizSpa,const int& style, ID3D11DeviceContext* context)
{
	textlen=wcslen(text);
	line=0; pos=0;
	vertexList.resize(textlen*4);
	for(int i=0;i<vertexList.size();i++){
		vertexList[i].Pos=XMFLOAT2(0,0);
		vertexList[i].Tex=XMFLOAT2(0,0);
	}
	for(int i=0;i<vertexList.size();i+=4){

		FLOAT leftX=0.0f,rightX=(FLOAT)fontStyles[style].charSize,upperY=0.0f,lowerY=(FLOAT)fontStyles[style].charSize;
		BOOL compatible=FALSE;
		
		
		if(text[i/4]==10){
			line+=fontStyles[style].recSize;
			pos=0;
		}
		else if(text[i/4]==32){
			pos+=fontStyles[style].recSize+sizSpa.x+sizSpa.y;
		}
		else if(fontStyles[style].lookup[text[i/4]].code==text[i/4]){
			leftX+=fontStyles[style].lookup[text[i/4]].offX*(FLOAT)fontStyles[style].charSize;
			rightX+=fontStyles[style].lookup[text[i/4]].offX*(FLOAT)fontStyles[style].charSize;
			upperY+=fontStyles[style].lookup[text[i/4]].offY*(FLOAT)fontStyles[style].charSize;
			lowerY+=fontStyles[style].lookup[text[i/4]].offY*(FLOAT)fontStyles[style].charSize;
			compatible=TRUE;
		}

		if(compatible){
			leftX/=(FLOAT)fontStyles[style].texWidth;
			rightX/=(FLOAT)fontStyles[style].texWidth;
			upperY/=(FLOAT)fontStyles[style].texHeight;
			lowerY/=(FLOAT)fontStyles[style].texHeight;

			vertexList[i].Pos=XMFLOAT2(pos+0-sizSpa.x*0.5f,0-line+sizSpa.x*0.5f); vertexList[i].Tex=XMFLOAT2(leftX,upperY);
			vertexList[i+1].Pos=XMFLOAT2(pos+fontStyles[style].recSize+sizSpa.x*0.5f,0-line+sizSpa.x*0.5f); vertexList[i+1].Tex=XMFLOAT2(rightX,upperY);
			vertexList[i+2].Pos=XMFLOAT2(pos+0-sizSpa.x*0.5f,-fontStyles[style].recSize-line-sizSpa.x*0.5f); vertexList[i+2].Tex=XMFLOAT2(leftX,lowerY);
			vertexList[i+3].Pos=XMFLOAT2(pos+fontStyles[style].recSize+sizSpa.x*0.5f,-fontStyles[style].recSize-line-sizSpa.x*0.5f); vertexList[i+3].Tex=XMFLOAT2(rightX,lowerY);

			pos+=fontStyles[style].recSize+sizSpa.x+sizSpa.y;
		}
	}
	//Renderer::immediateContext->UpdateSubresource( vertexBuffer, 0, NULL, vertexList.data(), 0, 0 );

	Renderer::UpdateBuffer(vertexBuffer,vertexList.data(),context==nullptr?Renderer::immediateContext:context,sizeof(Vertex) * textlen * 4);
	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//Vertex* dataPtr;
	//Renderer::immediateContext->Map(vertexBuffer,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
	//dataPtr = (Vertex*)mappedResource.pData;
	//memcpy(dataPtr,vertexList.data(),sizeof(Vertex) * textlen * 4);
	//Renderer::immediateContext->Unmap(vertexBuffer,0);
}

void Font::LoadVertexBuffer()
{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( Vertex ) * MAX_TEXT * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Renderer::graphicsDevice->CreateBuffer( &bd, NULL, &vertexBuffer );
}
void Font::LoadIndices()
{
	std::vector<unsigned long>indices;
	indices.resize(MAX_TEXT*6);
	for(unsigned long i=0;i<MAX_TEXT*4;i+=4){
		indices[i/4*6+0]=i/4*4+0;
		indices[i/4*6+1]=i/4*4+2;
		indices[i/4*6+2]=i/4*4+1;
		indices[i/4*6+3]=i/4*4+1;
		indices[i/4*6+4]=i/4*4+2;
		indices[i/4*6+5]=i/4*4+3;
	}
	
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( unsigned long ) * MAX_TEXT * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = indices.data();
	Renderer::graphicsDevice->CreateBuffer( &bd, &InitData, &indexBuffer );
}


void Font::DrawBlink(wchar_t* text, XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context)
{
	if(toDraw){
		Draw(text,newPosSizSpa,Halign,Valign,context);
	}
}
void Font::DrawBlink(const std::string& text, XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context)
{
	if(toDraw){
		Draw(text,newPosSizSpa,Halign,Valign,context);
	}
}
void Font::Draw(const std::string& text, XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context)
{
	std::wstring ws(text.begin(), text.end());

	Draw(ws.c_str(),newPosSizSpa,Halign,Valign,context);
}
void Font::Draw(const wchar_t* text, XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context)
{
	Draw(text,"",newPosSizSpa,Halign,Valign,context);
}

void Font::DrawBlink(const string& text,const char* fontStyle,XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context){
	if(toDraw){
		Draw(text,fontStyle,newPosSizSpa,Halign,Valign,context);
	}
}
void Font::DrawBlink(const wchar_t* text,const char* fontStyle,XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context){
	if(toDraw){
		Draw(text,fontStyle,newPosSizSpa,Halign,Valign,context);
	}
}
void Font::Draw(const string& text,const char* fontStyle,XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context){
	wstring ws(text.begin(),text.end());
	Draw(ws.c_str(),fontStyle,newPosSizSpa,Halign,Valign,context);
}


void Font::Draw(const wchar_t* text,const char* fontStyle,XMFLOAT4 newPosSizSpa, const char* Halign, const char* Valign, Renderer::DeviceContext context){
	int fontStyleI = getFontStyleByName(fontStyle);


	if(!strcmp(Halign,"center") || !strcmp(Valign,"mid"))
		newPosSizSpa.x-=textWidth(text,newPosSizSpa.w+newPosSizSpa.z,fontStyleI)/2;
	else if(!strcmp(Halign,"right"))
		newPosSizSpa.x-=textWidth(text,newPosSizSpa.w+newPosSizSpa.z,fontStyleI);
	if(!strcmp(Valign,"center") || !strcmp(Valign,"mid"))
		newPosSizSpa.y+=textHeight(text,newPosSizSpa.z,fontStyleI)*0.5f;
	else if(!strcmp(Valign,"bottom"))
		newPosSizSpa.y+=textHeight(text,newPosSizSpa.z,fontStyleI);

	
	ModifyGeo(text,XMFLOAT2(newPosSizSpa.z,newPosSizSpa.w),fontStyleI,context);

	if(textlen){

		if(context==nullptr)
			context=Renderer::immediateContext;
	
		Renderer::BindPrimitiveTopology(Renderer::PRIMITIVETOPOLOGY::TRIANGLELIST,context);
		Renderer::BindVertexLayout(vertexLayout,context);
		Renderer::BindVS(vertexShader,context);
		Renderer::BindPS(pixelShader,context);


		ConstantBuffer* cb = new ConstantBuffer();
		cb->mProjection = XMMatrixTranspose( cam->Oprojection );
		cb->mTrans =  XMMatrixTranspose( XMMatrixTranslation(newPosSizSpa.x,newPosSizSpa.y,0) );
		cb->mDimensions = XMFLOAT4(RENDERWIDTH,RENDERHEIGHT,0,0);
		
		Renderer::UpdateBuffer(constantBuffer,cb,context);
		delete cb;

		Renderer::BindConstantBufferVS(constantBuffer,0,context);

		Renderer::BindRasterizerState(rasterizerState,context);
		Renderer::BindDepthStencilState(depthStencilState,1,context);

		Renderer::BindBlendState(blendState,context);
		Renderer::BindVertexBuffer(vertexBuffer,0,sizeof(Vertex),context);
		Renderer::BindIndexBuffer(indexBuffer,context);

		Renderer::BindTexturePS(fontStyles[fontStyleI].texture,0,context);
		Renderer::BindSamplerPS(sampleState,0,context);
		Renderer::DrawIndexed(textlen*6,context);
	}
}

void Font::Blink(DWORD perframe,DWORD invisibleTime)
{
	counter++;
	if(toDraw && counter>perframe){
		counter=0;
		toDraw=FALSE;
	}
	else if(!toDraw && counter>invisibleTime){
		counter=0;
		toDraw=TRUE;
	}
}


int Font::textWidth(const wchar_t* text,FLOAT spacing,const int& style)
{
	int i=0;
	int max=0,lineW=0;
	int len=wcslen(text);
	while(i<len){
		if(text[i]==10) {//ENDLINE
			if(max<lineW) max=lineW;
			lineW=0;
		}
		else lineW++;
		i++;
	}
	if(max==0) max=lineW;

	return max*(fontStyles[style].recSize+spacing);
}
int Font::textHeight(const wchar_t* text,FLOAT siz,const int& style)
{
	int i=0;
	int lines=1;
	int len=wcslen(text);
	while(i<len){
		if(text[i]==10) {//ENDLINE
			lines++;
		}
		i++;
	}

	return lines*(fontStyles[style].recSize+siz);
}




Font::FontStyle::FontStyle(const string& newName){
	name=newName;

	for(short i=0;i<127;i++) lookup[i].code=lookup[i].offX=lookup[i].offY=0;

	std::stringstream ss(""),ss1("");
	ss<<"fonts/"<<name<<".txt";
	ss1<<"fonts/"<<name<<".dds";
	std::ifstream file(ss.str());
	if(file.is_open()){
		//CreateWICTextureFromFile(FALSE,Renderer::graphicsDevice,0,wss1.str().c_str(),0,&texture,0);
		texture = (Renderer::TextureView)ResourceManager::add(ss1.str());
		file>>texWidth>>texHeight>>recSize>>charSize;
		int i=0;
		while(!file.eof()){
			i++;
			int code=0;
			file>>code;
			lookup[code].code=code;
			file>>lookup[code].offX>>lookup[code].offY>>lookup[code].width;
		}
		file.close();
	}
	else {MessageBox(NULL,L"Could not load Font Data!",L"Error!",0); exit(0);}
}
void Font::FontStyle::CleanUp(){
	if(texture) texture->Release(); texture=NULL;
}
void Font::addFontStyle( string toAdd){
	MUTEX.lock();
	fontStyles.push_back(FontStyle(toAdd));
	MUTEX.unlock();
}
int Font::getFontStyleByName( string get){
	for(int i=0;i<fontStyles.size();i++)
	if(!strcmp(fontStyles[i].name.c_str(),get.c_str()))
		return i;
	return 0;
}
