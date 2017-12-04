#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "wiLoader.h"
#include "ResourceMapping.h"

#include <fstream>

using namespace std;
using namespace wiGraphicsTypes;

#define MAX_TEXT 10000

#define WHITESPACE_SIZE 3

std::string			wiFont::FONTPATH = "fonts/";
GPURingBuffer		*wiFont::vertexBuffer = nullptr;
GPUBuffer			*wiFont::indexBuffer = nullptr;
VertexLayout		*wiFont::vertexLayout = nullptr;
VertexShader		*wiFont::vertexShader = nullptr;
PixelShader			*wiFont::pixelShader = nullptr;
BlendState			*wiFont::blendState = nullptr;
RasterizerState		*wiFont::rasterizerState = nullptr;
DepthStencilState	*wiFont::depthStencilState = nullptr;
GraphicsPSO			*wiFont::PSO = nullptr;
std::vector<wiFont::wiFontStyle> wiFont::fontStyles;

wiFont::wiFont(const std::string& text, wiFontProps props, int style) : props(props), style(style)
{
	this->text = wstring(text.begin(), text.end());
}
wiFont::wiFont(const std::wstring& text, wiFontProps props, int style) : text(text), props(props), style(style)
{

}
wiFont::~wiFont()
{

}

void wiFont::Initialize()
{
}


void wiFont::LoadVertexBuffer()
{
	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DYNAMIC;
	bd.ByteWidth = 256 * 1024; // just allocate 256KB to font renderer ring buffer..
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;

	vertexBuffer = new GPURingBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, NULL, vertexBuffer);
}
void wiFont::LoadIndices()
{
	uint16_t indices[MAX_TEXT * 6];
	for (uint16_t i = 0; i < MAX_TEXT * 4; i += 4) {
		indices[i / 4 * 6 + 0] = i + 0;
		indices[i / 4 * 6 + 1] = i + 2;
		indices[i / 4 * 6 + 2] = i + 1;
		indices[i / 4 * 6 + 3] = i + 1;
		indices[i / 4 * 6 + 4] = i + 2;
		indices[i / 4 * 6 + 5] = i + 3;
	}

	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = indices;
	indexBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, indexBuffer);
}

void wiFont::SetUpStates()
{
	RasterizerStateDesc rs;
	rs.FillMode = FILL_SOLID;
	rs.CullMode = CULL_BACK;
	rs.FrontCounterClockwise = true;
	rs.DepthBias = 0;
	rs.DepthBiasClamp = 0;
	rs.SlopeScaledDepthBias = 0;
	rs.DepthClipEnable = false;
	rs.MultisampleEnable = false;
	rs.AntialiasedLineEnable = false;
	rasterizerState = new RasterizerState;
	wiRenderer::GetDevice()->CreateRasterizerState(&rs, rasterizerState);




	
	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	depthStencilState = new DepthStencilState;
	wiRenderer::GetDevice()->CreateDepthStencilState(&dsd, depthStencilState);


	
	BlendStateDesc bd;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	blendState = new BlendState;
	wiRenderer::GetDevice()->CreateBlendState(&bd,blendState);
}
void wiFont::LoadShaders()
{

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, FORMAT_R16G16_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
	if (vsinfo != nullptr){
		vertexShader = vsinfo->vertexShader;
		vertexLayout = vsinfo->vertexLayout;
	}


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fontPS.cso", wiResourceManager::PIXELSHADER));


	GraphicsPSODesc desc;
	desc.vs = vertexShader;
	desc.ps = pixelShader;
	desc.il = vertexLayout;
	desc.bs = blendState;
	desc.rs = rasterizerState;
	desc.dss = depthStencilState;
	desc.numRTs = 1;
	desc.RTFormats[0] = GraphicsDevice::GetBackBufferFormat();
	SAFE_DELETE(PSO);
	PSO = new GraphicsPSO;
	wiRenderer::GetDevice()->CreateGraphicsPSO(&desc, PSO);
}
void wiFont::SetUpStaticComponents()
{
	SetUpStates();
	LoadShaders();
	LoadVertexBuffer();
	LoadIndices();

	// add default font:
	addFontStyle("default_font");
}
void wiFont::CleanUpStatic()
{
	for(unsigned int i=0;i<fontStyles.size();++i) 
		fontStyles[i].CleanUp();
	fontStyles.clear();

	SAFE_DELETE(vertexBuffer);
	SAFE_DELETE(indexBuffer);
	SAFE_DELETE(vertexLayout);
	SAFE_DELETE(vertexShader);
	SAFE_DELETE(pixelShader);
	SAFE_DELETE(blendState);
	SAFE_DELETE(rasterizerState);
	SAFE_DELETE(depthStencilState);
	SAFE_DELETE(vertexBuffer);
}


void wiFont::ModifyGeo(volatile Vertex* vertexList, const std::wstring& text, wiFontProps props, int style)
{
	size_t vertexCount = text.length() * 4;

	const int lineHeight = (props.size < 0 ? fontStyles[style].lineHeight : props.size);
	const float relativeSize = (props.size < 0 ? 1 : (float)props.size / (float)fontStyles[style].lineHeight);

	int line = 0;
	int pos = 0;
	for (unsigned int i = 0; i<vertexCount; i += 4)
	{
		bool compatible = false;
		wiFontStyle::LookUp lookup;
		
		if (text[i / 4] == '\n') 
		{
			line += lineHeight + props.spacingY;
			pos = 0;
		}
		else if (text[i / 4] == ' ') 
		{
			pos += WHITESPACE_SIZE + props.spacingX;
		}
		else if (text[i / 4] == '\t') 
		{
			pos += (WHITESPACE_SIZE + props.spacingX) * 5;
		}
		else if (text[i / 4] < ARRAYSIZE(fontStyles[style].lookup) && fontStyles[style].lookup[text[i / 4]].character == text[i / 4]) 
		{
			lookup = fontStyles[style].lookup[text[i / 4]];
			compatible = true;
		}

		HALF h0 = XMConvertFloatToHalf(0.0f);

		if (compatible) 
		{
			int characterWidth = (int)(lookup.pixelWidth * relativeSize);

			HALF h1 = XMConvertFloatToHalf(1.0f);
			HALF hl = XMConvertFloatToHalf(lookup.left);
			HALF hr = XMConvertFloatToHalf(lookup.right);

			vertexList[i + 0].Pos.x = (float)pos;
			vertexList[i + 0].Pos.y = (float)line;
			vertexList[i + 1].Pos.x = (float)pos + (float)characterWidth;
			vertexList[i + 1].Pos.y = (float)line;
			vertexList[i + 2].Pos.x = (float)pos;
			vertexList[i + 2].Pos.y = (float)line + (float)lineHeight;
			vertexList[i + 3].Pos.x = (float)pos + (float)characterWidth;
			vertexList[i + 3].Pos.y = (float)line + (float)lineHeight;

			vertexList[i + 0].Tex.x = hl;
			vertexList[i + 0].Tex.y = h0;
			vertexList[i + 1].Tex.x = hr;
			vertexList[i + 1].Tex.y = h0;
			vertexList[i + 2].Tex.x = hl;
			vertexList[i + 2].Tex.y = h1;
			vertexList[i + 3].Tex.x = hr;
			vertexList[i + 3].Tex.y = h1;

			pos += characterWidth + props.spacingX;
		}
		else
		{
			vertexList[i + 0].Pos.x = 0;
			vertexList[i + 0].Pos.y = 0;
			vertexList[i + 0].Tex.x = h0;
			vertexList[i + 0].Tex.y = h0;
			vertexList[i + 1].Pos.x = 0;
			vertexList[i + 1].Pos.y = 0;
			vertexList[i + 1].Tex.x = h0;
			vertexList[i + 1].Tex.y = h0;
			vertexList[i + 2].Pos.x = 0;
			vertexList[i + 2].Pos.y = 0;
			vertexList[i + 2].Tex.x = h0;
			vertexList[i + 2].Tex.y = h0;
			vertexList[i + 3].Pos.x = 0;
			vertexList[i + 3].Pos.y = 0;
			vertexList[i + 3].Tex.x = h0;
			vertexList[i + 3].Tex.y = h0;
		}
	}
}


void wiFont::Draw(GRAPHICSTHREAD threadID)
{
	if (text.length() <= 0)
	{
		return;
	}

	wiFontProps newProps = props;

	if (props.h_align == WIFALIGN_CENTER || props.h_align == WIFALIGN_MID)
		newProps.posX -= textWidth() / 2;
	else if (props.h_align == WIFALIGN_RIGHT)
		newProps.posX -= textWidth();
	if (props.v_align == WIFALIGN_CENTER || props.h_align == WIFALIGN_MID)
		newProps.posY -= textHeight() / 2;
	else if (props.v_align == WIFALIGN_BOTTOM)
		newProps.posY -= textHeight();



	GraphicsDevice* device = wiRenderer::GetDevice();
	device->EventBegin("Font", threadID);

	device->BindPrimitiveTopology(PRIMITIVETOPOLOGY::TRIANGLELIST, threadID);

	device->BindGraphicsPSO(PSO, threadID);

	UINT vboffset;
	volatile Vertex* textBuffer = (volatile Vertex*)device->AllocateFromRingBuffer(vertexBuffer, sizeof(Vertex) * text.length() * 4, vboffset, threadID);
	if (textBuffer == nullptr)
	{
		return;
	}
	ModifyGeo(textBuffer, text, newProps, style);
	device->InvalidateBufferAccess(vertexBuffer, threadID);

	const GPUBuffer* vbs[] = {
		vertexBuffer,
	};
	const UINT strides[] = {
		sizeof(Vertex),
	};
	const UINT offsets[] = {
		vboffset,
	};
	device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);

	assert(text.length() * 4 < 65536 && "The index buffer currently only supports so many characters!");
	device->BindIndexBuffer(indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

	device->BindResourcePS(fontStyles[style].texture, TEXSLOT_ONDEMAND0, threadID);

	wiRenderer::MiscCB cb;

	if (newProps.shadowColor.a > 0)
	{
		// font shadow render:
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation((float)newProps.posX + 1, (float)newProps.posY + 1, 0)
			* device->GetScreenProjection()
		);
		cb.mColor = XMFLOAT4(newProps.shadowColor.R, newProps.shadowColor.G, newProps.shadowColor.B, newProps.shadowColor.A);
		device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);

		device->DrawIndexed((int)text.length() * 6, 0, 0, threadID);
	}

	// font base render:
	cb.mTransform = XMMatrixTranspose(
		XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
		* device->GetScreenProjection()
	);
	cb.mColor = XMFLOAT4(newProps.color.R, newProps.color.G, newProps.color.B, newProps.color.A);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);

	device->DrawIndexed((int)text.length() * 6, 0, 0, threadID);

	device->EventEnd(threadID);
}



int wiFont::textWidth()
{
	int maxWidth = 0;
	int currentLineWidth = 0;
	const float relativeSize = (props.size < 0 ? 1.0f : (float)props.size / (float)fontStyles[style].lineHeight);
	for (size_t i = 0; i < text.length(); ++i)
	{
		if (text[i] == '\n')
		{
			currentLineWidth = 0;
		}
		else if (text[i] == ' ')
		{
			currentLineWidth += WHITESPACE_SIZE + props.spacingX;
		}
		else if (text[i] == '\t')
		{
			currentLineWidth += (WHITESPACE_SIZE + props.spacingX) * 5;
		}
		else
		{
			int characterWidth = (int)(fontStyles[style].lookup[text[i]].pixelWidth * relativeSize);
			currentLineWidth += characterWidth + props.spacingX;
		}
		maxWidth = max(maxWidth, currentLineWidth);
	}

	return maxWidth;
}
int wiFont::textHeight()
{
	int i=0;
	int lines=1;
	int len=(int)text.length();
	while(i<len)
	{
		if(text[i]=='\n')
		{
			lines++;
		}
		i++;
	}

	const int lineHeight = (props.size < 0 ? fontStyles[style].lineHeight : props.size);
	return lines*(lineHeight + props.spacingY);
}


void wiFont::SetText(const std::string& text)
{
	this->text = wstring(text.begin(), text.end());
}
void wiFont::SetText(const std::wstring& text)
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

wiFont::wiFontStyle::wiFontStyle(const std::string& newName)
{
	name=newName;

	ZeroMemory(lookup, sizeof(lookup));

	std::stringstream ss(""),ss1("");
	ss<<FONTPATH<<name<<".wifont";
	ss1<<FONTPATH<<name<<".dds";
	std::ifstream file(ss.str());
	if(file.is_open())
	{
		texture = (Texture2D*)wiResourceManager::GetGlobal()->add(ss1.str());
		if (texture == nullptr)
		{
			return;
		}
		texWidth = texture->GetDesc().Width;
		texHeight = texture->GetDesc().Height;

		string voidStr;
		file >> voidStr >> lineHeight;
		while (!file.eof())
		{
			int code = 0;
			file >> code;
			lookup[code].ascii = code;
			file >> lookup[code].character >> lookup[code].left >> lookup[code].right >> lookup[code].pixelWidth;
		}


		file.close();
	}
	else 
	{
		wiHelper::messageBox(name, "Could not load Font Data: " + ss.str());
	}
}
void wiFont::wiFontStyle::CleanUp(){
	SAFE_DELETE(texture);
}
void wiFont::addFontStyle( const std::string& toAdd ){
	for (auto& x : fontStyles)
	{
		if (!x.name.compare(toAdd))
			return;
	}
	fontStyles.push_back(wiFontStyle(toAdd));
}
int wiFont::getFontStyleByName( const std::string& get ){
	for (unsigned int i = 0; i<fontStyles.size(); i++)
	if(!fontStyles[i].name.compare(get))
		return i;
	return 0;
}

void wiFont::CleanUp()
{

}
