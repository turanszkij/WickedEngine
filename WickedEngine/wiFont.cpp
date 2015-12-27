#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "wiLoader.h"


ID3D11Buffer		*wiFont::vertexBuffer,*wiFont::indexBuffer;
ID3D11InputLayout   *wiFont::vertexLayout;
ID3D11VertexShader  *wiFont::vertexShader;
ID3D11PixelShader   *wiFont::pixelShader;
ID3D11BlendState*		wiFont::blendState;
ID3D11Buffer*           wiFont::constantBuffer;
ID3D11SamplerState*			wiFont::sampleState;
ID3D11RasterizerState*		wiFont::rasterizerState;
ID3D11DepthStencilState*	wiFont::depthStencilState;
vector<wiFont::Vertex> wiFont::vertexList;
vector<wiFont::wiFontStyle> wiFont::fontStyles;

wiFont::wiFont(const string& text, wiFontProps props, int style) : props(props), style(style)
{
	this->text = wstring(text.begin(), text.end());
}
wiFont::wiFont(const wstring& text, wiFontProps props, int style) : text(text), props(props), style(style)
{

}
wiFont::~wiFont()
{

}

void wiFont::Initialize()
{
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

void wiFont::SetUpStates()
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
	wiRenderer::graphicsDevice->CreateSamplerState(&samplerDesc, &sampleState);



	
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
	wiRenderer::graphicsDevice->CreateRasterizerState(&rs,&rasterizerState);




	
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
	wiRenderer::graphicsDevice->CreateDepthStencilState(&dsd, &depthStencilState);


	
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	wiRenderer::graphicsDevice->CreateBlendState(&bd,&blendState);
}
void wiFont::SetUpCB()
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &constantBuffer );
}
void wiFont::LoadShaders()
{

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	wiRenderer::VertexShaderInfo* vsinfo = static_cast<wiRenderer::VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
		vertexLayout = vsinfo->vertexLayout;
	}


	pixelShader = static_cast<wiRenderer::PixelShader>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontPS.cso", wiResourceManager::PIXELSHADER));









 //   ID3DBlob* pVSBlob = NULL;

	//if(FAILED(D3DReadFileToBlob(L"shaders/fontVS.cso", &pVSBlob))){MessageBox(0,L"Failed To load fontVS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &vertexShader );
	
	


 //   // Define the input layout
 //   D3D11_INPUT_ELEMENT_DESC layout[] =
 //   {
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
 //   };
	//UINT numElements = ARRAYSIZE( layout );
	//
 //   // Create the input layout
	//wiRenderer::graphicsDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
 //                                         pVSBlob->GetBufferSize(), &vertexLayout );
	//pVSBlob->Release();

 //   

	//
	//ID3DBlob* pPSBlob = NULL;

	//if(FAILED(D3DReadFileToBlob(L"shaders/fontPS.cso", &pPSBlob))){MessageBox(0,L"Failed To load fontPS.cso",0,0);}
	//wiRenderer::graphicsDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &pixelShader );
	//pPSBlob->Release();
}
void wiFont::SetUpStaticComponents()
{
	SetUpStates();
	SetUpCB();
	LoadShaders();
	LoadVertexBuffer();
	LoadIndices();
}
void wiFont::CleanUpStatic()
{
	for(unsigned int i=0;i<fontStyles.size();++i) 
		fontStyles[i].CleanUp();
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


void wiFont::ModifyGeo(const wstring& text, wiFontProps props, int style, ID3D11DeviceContext* context)
{
	int line = 0, pos = 0;
	vertexList.resize(text.length()*4);
	for(unsigned int i=0;i<vertexList.size();i++){
		vertexList[i].Pos=XMFLOAT2(0,0);
		vertexList[i].Tex=XMFLOAT2(0,0);
	}
	for (unsigned int i = 0; i<vertexList.size(); i += 4){

		float leftX=0.0f,rightX=(float)fontStyles[style].charSize,upperY=0.0f,lowerY=(float)fontStyles[style].charSize;
		bool compatible=FALSE;
		
		
		if (text[i / 4] == '\n'){
			line += (short)(fontStyles[style].recSize + props.size + props.spacingY);
			pos = 0;
		}
		else if (text[i / 4] == ' '){
			pos += (short)(fontStyles[style].recSize + props.size + props.spacingX);
		}
		else if (text[i / 4] == '\t'){
			pos += (short)((fontStyles[style].recSize + props.size + props.spacingX) * 5);
		}
		else if(text[i / 4] < ARRAYSIZE(fontStyles[style].lookup) && fontStyles[style].lookup[text[i/4]].code==text[i/4]){
			leftX += fontStyles[style].lookup[text[i / 4]].offX*(float)fontStyles[style].charSize;
			rightX += fontStyles[style].lookup[text[i / 4]].offX*(float)fontStyles[style].charSize;
			upperY += fontStyles[style].lookup[text[i / 4]].offY*(float)fontStyles[style].charSize;
			lowerY += fontStyles[style].lookup[text[i / 4]].offY*(float)fontStyles[style].charSize;
			compatible=TRUE;
		}

		if(compatible){
			leftX /= (float)fontStyles[style].texWidth;
			rightX /= (float)fontStyles[style].texWidth;
			upperY /= (float)fontStyles[style].texHeight;
			lowerY /= (float)fontStyles[style].texHeight;

			vertexList[i].Pos = XMFLOAT2(pos + 0 - props.size*0.5f, 0 - line + props.size*0.5f); vertexList[i].Tex = XMFLOAT2(leftX, upperY);
			vertexList[i + 1].Pos = XMFLOAT2(pos + fontStyles[style].recSize + props.size*0.5f, 0 - line + props.size*0.5f); vertexList[i + 1].Tex = XMFLOAT2(rightX, upperY);
			vertexList[i + 2].Pos = XMFLOAT2(pos + 0 - props.size*0.5f, -fontStyles[style].recSize - line - props.size*0.5f); vertexList[i + 2].Tex = XMFLOAT2(leftX, lowerY);
			vertexList[i + 3].Pos = XMFLOAT2(pos + fontStyles[style].recSize + props.size*0.5f, -fontStyles[style].recSize - line - props.size*0.5f); vertexList[i + 3].Tex = XMFLOAT2(rightX, lowerY);

			pos += (short)(fontStyles[style].recSize + props.size + props.spacingX);
		}
	}

	wiRenderer::UpdateBuffer(vertexBuffer,vertexList.data(),context==nullptr?wiRenderer::getImmediateContext():context,sizeof(Vertex) * text.length() * 4);
	
}

void wiFont::LoadVertexBuffer()
{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( Vertex ) * MAX_TEXT * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &vertexBuffer );
}
void wiFont::LoadIndices()
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
	wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &indexBuffer );
}

void wiFont::Draw(wiRenderer::DeviceContext context){

	wiFontProps newProps = props;

	if(props.h_align==WIFALIGN_CENTER || props.h_align==WIFALIGN_MID)
		newProps.posX-= textWidth()*0.5f;
	else if(props.h_align==WIFALIGN_RIGHT)
		newProps.posX -= textWidth();
	if (props.v_align == WIFALIGN_CENTER || props.h_align == WIFALIGN_MID)
		newProps.posY += textHeight()*0.5f;
	else if(props.v_align==WIFALIGN_BOTTOM)
		newProps.posY += textHeight();

	
	ModifyGeo(text, newProps, style, context);

	if(text.length()>0){

		if(context==nullptr)
			context=wiRenderer::getImmediateContext();
		if (context == nullptr)
			return;
	
		wiRenderer::BindPrimitiveTopology(wiRenderer::PRIMITIVETOPOLOGY::TRIANGLELIST,context);
		wiRenderer::BindVertexLayout(vertexLayout,context);
		wiRenderer::BindVS(vertexShader,context);
		wiRenderer::BindPS(pixelShader,context);


		static thread_local ConstantBuffer* cb = new ConstantBuffer;
		(*cb).mProjection = XMMatrixTranspose( XMMatrixOrthographicLH((float)wiRenderer::GetScreenWidth(),(float)wiRenderer::GetScreenHeight(),0,100) );
		(*cb).mTrans = XMMatrixTranspose(XMMatrixTranslation(newProps.posX, newProps.posY, 0));
		(*cb).mDimensions = XMFLOAT4((float)wiRenderer::RENDERWIDTH, (float)wiRenderer::RENDERHEIGHT, 0, 0);
		
		wiRenderer::UpdateBuffer(constantBuffer,cb,context);

		wiRenderer::BindConstantBufferVS(constantBuffer,0,context);

		wiRenderer::BindRasterizerState(rasterizerState,context);
		wiRenderer::BindDepthStencilState(depthStencilState,1,context);

		wiRenderer::BindBlendState(blendState,context);
		wiRenderer::BindVertexBuffer(vertexBuffer,0,sizeof(Vertex),context);
		wiRenderer::BindIndexBuffer(indexBuffer,context);

		wiRenderer::BindTexturePS(fontStyles[style].texture,0,context);
		wiRenderer::BindSamplerPS(sampleState,0,context);
		wiRenderer::DrawIndexed(text.length()*6,context);
	}
}



int wiFont::textWidth()
{
	int i=0;
	int max=0,lineW=0;
	int len=text.length();
	while(i<len){
		if(text[i]==10) {//ENDLINE
			if(max<lineW) max=lineW;
			lineW=0;
		}
		else lineW++;
		i++;
	}
	if(max==0) max=lineW;

	return (int)(max*(fontStyles[style].recSize+props.spacingX));
}
int wiFont::textHeight()
{
	int i=0;
	int lines=1;
	int len=text.length();
	while(i<len){
		if(text[i]==10) {//ENDLINE
			lines++;
		}
		i++;
	}

	return (int)(lines*(fontStyles[style].recSize+props.size));
}


void wiFont::SetText(const string& text)
{
	this->text = wstring(text.begin(), text.end());
}
void wiFont::SetText(const wstring& text)
{
	this->text = text;
}
wstring wiFont::GetText()
{
	return text;
}
string wiFont::GetTextA()
{
	return string(text.begin(),text.end());
}

wiFont::wiFontStyle::wiFontStyle(const string& newName){

	wiRenderer::Lock();

	name=newName;

	for(short i=0;i<127;i++) lookup[i].code=lookup[i].offX=lookup[i].offY=0;

	std::stringstream ss(""),ss1("");
	ss<<"fonts/"<<name<<".txt";
	ss1<<"fonts/"<<name<<".dds";
	std::ifstream file(ss.str());
	if(file.is_open()){
		texture = (wiRenderer::TextureView)wiResourceManager::GetGlobal()->add(ss1.str());
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
	else {
		wiHelper::messageBox(name,"Could not load Font Data!"); 
	}

	wiRenderer::Unlock();
}
void wiFont::wiFontStyle::CleanUp(){
	SAFE_RELEASE(texture);
}
void wiFont::addFontStyle( const string& toAdd ){
	for (auto& x : fontStyles)
	{
		if (!x.name.compare(toAdd))
			return;
	}
	fontStyles.push_back(wiFontStyle(toAdd));
}
int wiFont::getFontStyleByName( const string& get ){
	for (unsigned int i = 0; i<fontStyles.size(); i++)
	if(!fontStyles[i].name.compare(get))
		return i;
	return 0;
}

void wiFont::CleanUp()
{

}
