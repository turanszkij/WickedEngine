#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "wiLoader.h"
#include "ResourceMapping.h"

using namespace wiGraphicsTypes;


GPUBuffer			*wiFont::vertexBuffer = nullptr, *wiFont::indexBuffer = nullptr;
VertexLayout		*wiFont::vertexLayout = nullptr;
VertexShader		*wiFont::vertexShader = nullptr;
PixelShader			*wiFont::pixelShader = nullptr;
BlendState			*wiFont::blendState = nullptr;
GPUBuffer           *wiFont::constantBuffer = nullptr;
RasterizerState		*wiFont::rasterizerState = nullptr;
DepthStencilState	*wiFont::depthStencilState = nullptr;
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
}

void wiFont::SetUpStates()
{
	RasterizerStateDesc rs;
	rs.FillMode=FILL_SOLID;
	rs.CullMode=CULL_NONE;
	rs.FrontCounterClockwise=TRUE;
	rs.DepthBias=0;
	rs.DepthBiasClamp=0;
	rs.SlopeScaledDepthBias=0;
	rs.DepthClipEnable=FALSE;
	rs.ScissorEnable=FALSE;
	rs.MultisampleEnable=FALSE;
	rs.AntialiasedLineEnable=FALSE;
	rasterizerState = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs,rasterizerState);




	
	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = COMPARISON_LESS;

	dsd.StencilEnable = false;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	dsd.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	dsd.BackFace.StencilFailOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = COMPARISON_ALWAYS;

	// Create the depth stencil state.
	depthStencilState = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilState);


	
	BlendStateDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
	blendState = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendState);
}
void wiFont::SetUpCB()
{
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	constantBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, constantBuffer );

	wiRenderer::GetDevice()->LOCK();
	BindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
	wiRenderer::GetDevice()->UNLOCK();
}
void wiFont::LoadShaders()
{

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
		vertexLayout = vsinfo->vertexLayout;
	}


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontPS.cso", wiResourceManager::PIXELSHADER));

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
	
	//TODO
}

void wiFont::BindPersistentState(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->BindConstantBufferVS(constantBuffer, CB_GETBINDSLOT(ConstantBuffer), threadID);
}


void wiFont::ModifyGeo(const wstring& text, wiFontProps props, int style, GRAPHICSTHREAD threadID)
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

			vertexList[i + 0].Pos =	XMFLOAT2(pos + 0 - props.size*0.5f							, line - props.size*0.5f); 
			vertexList[i + 1].Pos = XMFLOAT2(pos + fontStyles[style].recSize + props.size*0.5f	, line - props.size*0.5f); 
			vertexList[i + 2].Pos = XMFLOAT2(pos + 0 - props.size*0.5f							, fontStyles[style].recSize + line + props.size*0.5f); 
			vertexList[i + 3].Pos = XMFLOAT2(pos + fontStyles[style].recSize + props.size*0.5f	, fontStyles[style].recSize + line + props.size*0.5f); 

			vertexList[i + 0].Tex = XMFLOAT2(leftX, upperY);
			vertexList[i + 1].Tex = XMFLOAT2(rightX, upperY);
			vertexList[i + 2].Tex = XMFLOAT2(leftX, lowerY);
			vertexList[i + 3].Tex = XMFLOAT2(rightX, lowerY);

			pos += (short)(fontStyles[style].recSize + props.size + props.spacingX);
		}
	}

	wiRenderer::GetDevice()->UpdateBuffer(vertexBuffer,vertexList.data(),threadID,sizeof(Vertex) * text.length() * 4);
	
}

void wiFont::LoadVertexBuffer()
{
		GPUBufferDesc bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( Vertex ) * MAX_TEXT * 4;
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;
		vertexBuffer = new GPUBuffer;
		wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, vertexBuffer );
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
	
	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof( unsigned long ) * MAX_TEXT * 6;
    bd.BindFlags = BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = indices.data();
	indexBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer( &bd, &InitData, indexBuffer );
}

void wiFont::Draw(GRAPHICSTHREAD threadID){

	wiFontProps newProps = props;

	if(props.h_align==WIFALIGN_CENTER || props.h_align==WIFALIGN_MID)
		newProps.posX-= textWidth()*0.5f;
	else if(props.h_align==WIFALIGN_RIGHT)
		newProps.posX -= textWidth();
	if (props.v_align == WIFALIGN_CENTER || props.h_align == WIFALIGN_MID)
		newProps.posY -= textHeight()*0.5f;
	else if(props.v_align==WIFALIGN_BOTTOM)
		newProps.posY -= textHeight();

	
	ModifyGeo(text, newProps, style, threadID);

	if(text.length()>0)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin(L"Font", threadID);
	
		device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST,threadID);
		device->BindVertexLayout(vertexLayout,threadID);
		device->BindVS(vertexShader,threadID);
		device->BindPS(pixelShader,threadID);


		ConstantBuffer cb;
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(newProps.posX, newProps.posY, 0)
			* XMMatrixOrthographicOffCenterLH(0, (float)device->GetScreenWidth(), (float)device->GetScreenHeight(), 0, -1, 1)
		);
		
		device->UpdateBuffer(constantBuffer,&cb,threadID);


		device->BindRasterizerState(rasterizerState,threadID);
		device->BindDepthStencilState(depthStencilState,1,threadID);

		device->BindBlendState(blendState,threadID);
		device->BindVertexBuffer(vertexBuffer,0,sizeof(Vertex),threadID);
		device->BindIndexBuffer(indexBuffer,threadID);

		device->BindResourcePS(fontStyles[style].texture,TEXSLOT_ONDEMAND0,threadID);
		device->DrawIndexed(text.length()*6,threadID);

		device->EventEnd(threadID);
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

	wiRenderer::GetDevice()->LOCK();

	name=newName;

	for(short i=0;i<127;i++) lookup[i].code=lookup[i].offX=lookup[i].offY=0;

	std::stringstream ss(""),ss1("");
	ss<<"fonts/"<<name<<".txt";
	ss1<<"fonts/"<<name<<".dds";
	std::ifstream file(ss.str());
	if(file.is_open()){
		texture = (Texture2D*)wiResourceManager::GetGlobal()->add(ss1.str());
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

	wiRenderer::GetDevice()->UNLOCK();
}
void wiFont::wiFontStyle::CleanUp(){
	SAFE_DELETE(texture);
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
