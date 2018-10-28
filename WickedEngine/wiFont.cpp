#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "ShaderInterop_Font.h"
#include "wiBackLog.h"
#include "wiTextureHelper.h"

#include "Utility/stb_truetype.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;

#define MAX_TEXT 10000

#define WHITESPACE_SIZE 3

namespace wiFont_Internal
{
	std::string			FONTPATH = "fonts/";
	GPURingBuffer		vertexBuffer;
	GPUBuffer			indexBuffer;
	GPUBuffer			constantBuffer;
	BlendState			blendState;
	RasterizerState		rasterizerState;
	DepthStencilState	depthStencilState;
	Sampler				sampler;

	VertexLayout		*vertexLayout = nullptr;
	VertexShader		*vertexShader = nullptr;
	PixelShader			*pixelShader = nullptr;
	GraphicsPSO			*PSO = nullptr;

	bool initialized = false;

	struct wiFontStyle
	{
		std::string name;
		wiGraphicsTypes::Texture2D* texture = nullptr;

		struct LookUp
		{
			char character;
			float left;
			float right;
			int pixelWidth;
		};
		LookUp lookup[128];
		int lineHeight;

		wiFontStyle() {}
		wiFontStyle(const std::string& newName, int height = 16) : name(newName), lineHeight(height)
		{
			size_t size;
			unsigned char* fontBuffer;
			wiHelper::readByteData(FONTPATH + newName + ".ttf", &fontBuffer, size);

			stbtt_fontinfo info;
			if (!stbtt_InitFont(&info, fontBuffer, 0))
			{
				wiHelper::messageBox("Failed to load font!");
				return;
			}

			const int textureWidth = 2048;
			const int textureHeight = lineHeight;

			// create a bitmap for the phrase
			unsigned char* bitmap = (unsigned char*)calloc(textureWidth * textureHeight, sizeof(unsigned char));

			// calculate font scaling
			float scale = stbtt_ScaleForPixelHeight(&info, (float)textureHeight);

			const int atlaspadding = 2;
			int x = 0;

			int ascent, descent, lineGap;
			stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

			ascent = int(float(ascent) * scale);
			descent = int(float(descent) * scale);

			for (int i = 0; i < ARRAYSIZE(lookup); ++i)
			{
				// get bounding box for character (may be offset to account for chars that dip above or below the line
				int c_x1, c_y1, c_x2, c_y2;
				stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

				// compute y (different characters have different heights
				int y = ascent + c_y1;

				// render character (stride and offset is important here)
				int byteOffset = x + (y  * textureWidth);
				stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, textureWidth, scale, scale, i);

				LookUp& lut = lookup[i];
				lut.character = i;
				lut.left = (float(x) + float(c_x1) - 0.5f) / float(textureWidth);
				lut.right = (float(x) + float(c_x2) + 0.5f) / float(textureWidth);

				// how wide is this character
				int ax;
				stbtt_GetCodepointHMetrics(&info, i, &ax, 0);
				x += int(float(ax) * scale);

				lut.pixelWidth = int(float(ax) * scale);

				// add kerning
				int kern;
				kern = stbtt_GetCodepointKernAdvance(&info, i, i + 1);
				x += int(float(kern) * scale);

				// add slight padding to atlas texture betwwen characters
				x += atlaspadding;
			}

			HRESULT hr = wiTextureHelper::CreateTexture(texture, bitmap, textureWidth, textureHeight, 1, FORMAT_R8_UNORM);
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(bitmap);
		}
		wiFontStyle(wiFontStyle&& other)
		{
			this->texture = other.texture;
			this->lineHeight = other.lineHeight;
			memcpy(this->lookup, other.lookup, sizeof(this->lookup));

			other.texture = nullptr;
		}
		~wiFontStyle()
		{
			SAFE_DELETE(texture);
		}
	};
	std::vector<wiFontStyle> fontStyles;

	struct FontVertex
	{
		XMFLOAT2 Pos;
		XMHALF2 Tex;
	};

	void ModifyGeo(volatile FontVertex* vertexList, const std::wstring& text, wiFontProps props, int style)
	{
		size_t vertexCount = text.length() * 4;

		const int lineHeight = (props.size < 0 ? fontStyles[style].lineHeight : props.size);
		const float relativeSize = (props.size < 0 ? 1 : (float)props.size / (float)fontStyles[style].lineHeight);

		int line = 0;
		int pos = 0;
		for (unsigned int i = 0; i < vertexCount; i += 4)
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

}
using namespace wiFont_Internal;


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
	if (initialized)
	{
		return;
	}

	// add default font:
	addFontStyle("arial", 16);

	GraphicsDevice* device = wiRenderer::GetDevice();

	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DYNAMIC;
		bd.ByteWidth = 256 * 1024; // just allocate 256KB to font renderer ring buffer..
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;

		HRESULT hr = device->CreateBuffer(&bd, nullptr, &vertexBuffer);
		assert(SUCCEEDED(hr));
	}

	{
		uint16_t indices[MAX_TEXT * 6];
		for (uint16_t i = 0; i < MAX_TEXT * 4; i += 4) 
		{
			indices[i / 4 * 6 + 0] = i + 0;
			indices[i / 4 * 6 + 1] = i + 2;
			indices[i / 4 * 6 + 2] = i + 1;
			indices[i / 4 * 6 + 3] = i + 1;
			indices[i / 4 * 6 + 4] = i + 2;
			indices[i / 4 * 6 + 5] = i + 3;
		}

		GPUBufferDesc bd;
		bd.Usage = USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(indices);
		bd.BindFlags = BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		SubresourceData InitData;
		InitData.pSysMem = indices;

		HRESULT hr = device->CreateBuffer(&bd, &InitData, &indexBuffer);
		assert(SUCCEEDED(hr));
	}

	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(FontCB);
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;

		HRESULT hr = device->CreateBuffer(&bd, nullptr, &constantBuffer);
		assert(SUCCEEDED(hr));
	}



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
	device->CreateRasterizerState(&rs, &rasterizerState);

	DepthStencilStateDesc dsd;
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	device->CreateDepthStencilState(&dsd, &depthStencilState);

	BlendStateDesc bd;
	bd.RenderTarget[0].BlendEnable = true;
	bd.RenderTarget[0].SrcBlend = BLEND_ONE;
	bd.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
	bd.IndependentBlendEnable = false;
	device->CreateBlendState(&bd, &blendState);

	SamplerDesc samplerDesc;
	samplerDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, &sampler);

	LoadShaders();

	wiBackLog::post("wiFont Initialized");
	initialized = true;
}
void wiFont::CleanUp()
{
	fontStyles.clear();

	SAFE_DELETE(vertexShader);
	SAFE_DELETE(pixelShader);
}

void wiFont::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R32G32_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, FORMAT_R16G16_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	vertexShader = static_cast<VertexShader*>(wiResourceManager::GetShaderManager()->add(path + "fontVS.cso", wiResourceManager::VERTEXSHADER));
	
	vertexLayout = new VertexLayout;
	wiRenderer::GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShader->code.data, vertexShader->code.size, vertexLayout);


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(path + "fontPS.cso", wiResourceManager::PIXELSHADER));


	GraphicsPSODesc desc;
	desc.vs = vertexShader;
	desc.ps = pixelShader;
	desc.il = vertexLayout;
	desc.bs = &blendState;
	desc.rs = &rasterizerState;
	desc.dss = &depthStencilState;
	desc.numRTs = 1;
	desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
	RECREATE(PSO);
	wiRenderer::GetDevice()->CreateGraphicsPSO(&desc, PSO);
}

void wiFont::BindPersistentState(GRAPHICSTHREAD threadID)
{
	if (!initialized)
	{
		return;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->BindConstantBuffer(VS, &constantBuffer, CB_GETBINDSLOT(FontCB), threadID);
	device->BindConstantBuffer(PS, &constantBuffer, CB_GETBINDSLOT(FontCB), threadID);
}


void wiFont::Draw(GRAPHICSTHREAD threadID)
{
	if (!initialized || text.length() <= 0)
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

	UINT vboffset;
	volatile FontVertex* textBuffer = (volatile FontVertex*)device->AllocateFromRingBuffer(&vertexBuffer, sizeof(FontVertex) * text.length() * 4, vboffset, threadID);
	if (textBuffer == nullptr)
	{
		return;
	}
	ModifyGeo(textBuffer, text, newProps, style);
	device->InvalidateBufferAccess(&vertexBuffer, threadID);

	device->EventBegin("Font", threadID);

	device->BindGraphicsPSO(PSO, threadID);
	device->BindSampler(PS, &sampler, SSLOT_ONDEMAND1, threadID);

	GPUBuffer* vbs[] = {
		&vertexBuffer,
	};
	const UINT strides[] = {
		sizeof(FontVertex),
	};
	const UINT offsets[] = {
		vboffset,
	};
	device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);

	assert(text.length() * 4 < 65536 && "The index buffer currently only supports so many characters!");
	device->BindIndexBuffer(&indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

	device->BindResource(PS, fontStyles[style].texture, TEXSLOT_ONDEMAND1, threadID);

	FontCB cb;

	if (newProps.shadowColor.a > 0)
	{
		// font shadow render:
		XMStoreFloat4x4(&cb.g_xFont_Transform, XMMatrixTranspose(
			XMMatrixTranslation((float)newProps.posX + 1, (float)newProps.posY + 1, 0)
			* device->GetScreenProjection()
		));
		cb.g_xFont_Color = float4(newProps.shadowColor.R, newProps.shadowColor.G, newProps.shadowColor.B, newProps.shadowColor.A);
		device->UpdateBuffer(&constantBuffer, &cb, threadID);

		device->DrawIndexed((int)text.length() * 6, 0, 0, threadID);
	}

	// font base render:
	XMStoreFloat4x4(&cb.g_xFont_Transform, XMMatrixTranspose(
		XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
		* device->GetScreenProjection()
	));
	cb.g_xFont_Color = float4(newProps.color.R, newProps.color.G, newProps.color.B, newProps.color.A);
	device->UpdateBuffer(&constantBuffer, &cb, threadID);

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

void wiFont::addFontStyle(const std::string& fontName, int height)
{
	for (auto& x : fontStyles)
	{
		if (!x.name.compare(fontName) && x.lineHeight == height)
		{
			return;
		}
	}
	fontStyles.push_back(wiFontStyle(fontName, height));
}
int wiFont::getFontStyleByName(const std::string& fontName, int height)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		if (!fontStyles[i].name.compare(fontName) && fontStyles[i].lineHeight == height)
		{
			return int(i);
		}
	}
	return 0;
}

std::string& wiFont::GetFontPath()
{
	return FONTPATH;
}
