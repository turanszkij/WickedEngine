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
#define TAB_WHITESPACECOUNT 4

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
		size_t fontBufferSize = 0;
		unsigned char* fontBuffer = nullptr;
		stbtt_fontinfo fontInfo;
		float fontScaling = 1;

		struct LookUp
		{
			float left;
			float right;
			int pixelWidth;
		};
		LookUp lookup[128];
		int lineHeight;

		wiFontStyle() {}
		wiFontStyle(const std::string& newName, int height = 16) : name(newName), lineHeight(height)
		{
			wiHelper::readByteData(FONTPATH + newName + ".ttf", &fontBuffer, fontBufferSize);

			if (!stbtt_InitFont(&fontInfo, fontBuffer, 0))
			{
				wiHelper::messageBox("Failed to load font!");
				return;
			}

			const int textureWidth = 2048;
			const int textureHeight = lineHeight;

			// create a bitmap for the phrase
			unsigned char* bitmap = new unsigned char[textureWidth * textureHeight];
			memset(bitmap, 0, textureWidth * textureHeight * sizeof(unsigned char));

			// calculate font scaling
			fontScaling = stbtt_ScaleForPixelHeight(&fontInfo, (float)textureHeight);

			const int atlaspadding = 2;
			int x = 0;

			int ascent, descent, lineGap;
			stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

			ascent = int(float(ascent) * fontScaling);
			descent = int(float(descent) * fontScaling);

			for (int i = 0; i < ARRAYSIZE(lookup); ++i)
			{
				// get bounding box for character (may be offset to account for chars that dip above or below the line
				int c_x1, c_y1, c_x2, c_y2;
				stbtt_GetCodepointBitmapBox(&fontInfo, i, fontScaling, fontScaling, &c_x1, &c_y1, &c_x2, &c_y2);

				// compute y (different characters have different heights
				int y = ascent + c_y1;

				// render character (stride and offset is important here)
				int byteOffset = x + (y  * textureWidth);
				stbtt_MakeCodepointBitmap(&fontInfo, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, textureWidth, fontScaling, fontScaling, i);

				LookUp& lut = lookup[i];
				lut.left = (float(x) + float(c_x1) - 0.5f) / float(textureWidth);
				lut.right = (float(x) + float(c_x2) + 0.5f) / float(textureWidth);

				// how wide is this character
				int ax;
				stbtt_GetCodepointHMetrics(&fontInfo, i, &ax, 0);
				lut.pixelWidth = int(float(ax) * fontScaling);

				// advance to next character packed pos
				x += lut.pixelWidth;

				// add slight padding to atlas texture between characters
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
			this->fontBufferSize = other.fontBufferSize;
			this->fontBuffer = other.fontBuffer;
			this->fontInfo = std::move(other.fontInfo);

			other.texture = nullptr;
			other.fontBuffer = nullptr;
		}
		~wiFontStyle()
		{
			SAFE_DELETE(texture);
			SAFE_DELETE_ARRAY(fontBuffer);
		}
	};
	std::vector<wiFontStyle> fontStyles;

	struct FontVertex
	{
		XMUSHORT2 Pos;
		XMHALF2 Tex;
	};

	int ModifyGeo(volatile FontVertex* vertexList, const std::wstring& text, wiFontProps props, int style)
	{
		int quadCount = 0;

		const wiFontStyle& fontStyle = fontStyles[style];

		const uint16_t lineHeight = (props.size < 0 ? uint16_t(fontStyle.lineHeight) : uint16_t(props.size));
		const float relativeSize = (props.size < 0 ? 1 : (float)props.size / (float)fontStyle.lineHeight);

		const HALF hzero = XMConvertFloatToHalf(0.0f);
		const HALF hone = XMConvertFloatToHalf(1.0f);

		uint16_t line = 0;
		uint16_t pos = 0;
		for (size_t i = 0; i < text.length(); ++i)
		{
			const wchar_t character = text[i];

			if (character == '\n')
			{
				line += lineHeight + props.spacingY;
				pos = 0;
			}
			else if (character == ' ')
			{
				pos += WHITESPACE_SIZE + props.spacingX;
			}
			else if (character == '\t')
			{
				pos += (WHITESPACE_SIZE + props.spacingX) * TAB_WHITESPACECOUNT;
			}
			else if (character < ARRAYSIZE(fontStyle.lookup))
			{
				const wiFontStyle::LookUp& lookup = fontStyle.lookup[character];
				uint16_t characterWidth = uint16_t(lookup.pixelWidth * relativeSize);

				const size_t vertexID = quadCount * 4;

				const uint16_t left = pos;
				const uint16_t right = pos + characterWidth;
				const uint16_t top = line;
				const uint16_t bottom = line + lineHeight;

				vertexList[vertexID + 0].Pos.x = left;
				vertexList[vertexID + 0].Pos.y = top;
				vertexList[vertexID + 1].Pos.x = right;
				vertexList[vertexID + 1].Pos.y = top;
				vertexList[vertexID + 2].Pos.x = left;
				vertexList[vertexID + 2].Pos.y = bottom;
				vertexList[vertexID + 3].Pos.x = right;
				vertexList[vertexID + 3].Pos.y = bottom;

				const HALF hleft = XMConvertFloatToHalf(lookup.left);
				const HALF hright = XMConvertFloatToHalf(lookup.right);

				vertexList[vertexID + 0].Tex.x = hleft;
				vertexList[vertexID + 0].Tex.y = hzero;
				vertexList[vertexID + 1].Tex.x = hright;
				vertexList[vertexID + 1].Tex.y = hzero;
				vertexList[vertexID + 2].Tex.x = hleft;
				vertexList[vertexID + 2].Tex.y = hone;
				vertexList[vertexID + 3].Tex.x = hright;
				vertexList[vertexID + 3].Tex.y = hone;

				pos += characterWidth + props.spacingX;

				//// add kerning
				//if (i > 0 && i < text.length() - 1)
				//{
				//	const wchar_t next_character = text[i + 1];
				//	if (next_character != ' ' && next_character != '\t')
				//	{
				//		int kern;
				//		kern = stbtt_GetCodepointKernAdvance(&fontStyle.fontInfo, character, next_character);
				//		pos += int(float(kern) * fontStyle.fontScaling);
				//	}
				//}

				quadCount++;
			}
		}

		return quadCount;
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

	// add default font if there is none yet:
	if (fontStyles.empty())
	{
		AddFontStyle("arial", 16);
	}

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
		{ "POSITION", 0, FORMAT_R16G16_UINT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
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
	const int quadCount = ModifyGeo(textBuffer, text, newProps, style);
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

		device->DrawIndexed(quadCount * 6, 0, 0, threadID);
	}

	// font base render:
	XMStoreFloat4x4(&cb.g_xFont_Transform, XMMatrixTranspose(
		XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
		* device->GetScreenProjection()
	));
	cb.g_xFont_Color = float4(newProps.color.R, newProps.color.G, newProps.color.B, newProps.color.A);
	device->UpdateBuffer(&constantBuffer, &cb, threadID);

	device->DrawIndexed(quadCount * 6, 0, 0, threadID);

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
			currentLineWidth += (WHITESPACE_SIZE + props.spacingX) * TAB_WHITESPACECOUNT;
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
	int i = 0;
	int lines = 1;
	int len = (int)text.length();
	while (i < len)
	{
		if (text[i] == '\n')
		{
			lines++;
		}
		i++;
	}

	const int lineHeight = (props.size < 0 ? fontStyles[style].lineHeight : props.size);
	return lines * (lineHeight + props.spacingY);
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

int wiFont::AddFontStyle(const std::string& fontName, int height)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		const wiFontStyle& fontStyle = fontStyles[i];
		if (!fontStyle.name.compare(fontName) && fontStyle.lineHeight == height)
		{
			return int(i);
		}
	}
	fontStyles.push_back(wiFontStyle(fontName, height));
	return int(fontStyles.size() - 1);
}
int wiFont::GetFontStyle(const std::string& fontName, int height)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		const wiFontStyle& fontStyle = fontStyles[i];
		if (!fontStyle.name.compare(fontName) && fontStyle.lineHeight == height)
		{
			return int(i);
		}
	}
	return -1; // fontstyle not found
}

std::string& wiFont::GetFontPath()
{
	return FONTPATH;
}
