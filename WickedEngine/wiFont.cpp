#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "ShaderInterop_Font.h"
#include "wiBackLog.h"
#include "wiTextureHelper.h"
#include "wiRectPacker.h"

#include "Utility/stb_truetype.h"

#include <fstream>
#include <sstream>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiRectPacker;

#define MAX_TEXT 10000
#define WHITESPACE_SIZE 3
#define TAB_WHITESPACECOUNT 4

namespace wiFont_Internal
{
	std::string			FONTPATH = "fonts/";
	GPURingBuffer		vertexBuffers[GRAPHICSTHREAD_COUNT];
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

	atomic_bool initialized = false;

	Texture2D* texture = nullptr;

	unordered_map<int64_t, rect_xywhf> rects;
	inline int64_t glyphhash(int code, int style) { return (int64_t(code) << 32) | int64_t(style); }
	inline int codefromhash(int64_t hash) { return int((hash >> 32) & 0xFFFFFFFF); }
	inline int stylefromhash(int64_t hash) { return int(hash & 0xFFFFFFFF); }
	unordered_set<int64_t> pendingGlyphs;

	struct wiFontStyle
	{
		string name;
		size_t fontBufferSize = 0;
		vector<uint8_t> fontBuffer;
		stbtt_fontinfo fontInfo;
		float fontScaling = 1;
		int lineHeight = 16;
		float ascent = 0;
		float descent = 0;
		float lineGap = 0;

		struct Glyph
		{
			int16_t x;
			int16_t y;
			int16_t width;
			int16_t height;
			uint16_t tc_left;
			uint16_t tc_right;
			uint16_t tc_top;
			uint16_t tc_bottom;
		};
		unordered_map<int, Glyph> lookup;

		wiFontStyle(const string& newName, int height = 16) : name(newName), lineHeight(height)
		{
			wiHelper::readByteData(newName, fontBuffer, fontBufferSize);

			if (!stbtt_InitFont(&fontInfo, fontBuffer.data(), 0))
			{
				stringstream ss("");
				ss << "Failed to load font: " << name;
				wiHelper::messageBox(ss.str());
				return;
			}

			fontScaling = stbtt_ScaleForPixelHeight(&fontInfo, (float)lineHeight);

			int _a, _d, _l;
			stbtt_GetFontVMetrics(&fontInfo, &_a, &_d, &_l);

			ascent = float(_a) * fontScaling;
			descent = float(_d) * fontScaling;
			lineGap = float(_l) * fontScaling;

		}
	};
	std::vector<wiFontStyle*> fontStyles;

	struct FontVertex
	{
		XMSHORT2 Pos;
		XMHALF2 Tex;
	};

	int ModifyGeo(volatile FontVertex* vertexList, const std::wstring& text, wiFontProps props, int style)
	{
		int quadCount = 0;

		const wiFontStyle& fontStyle = *fontStyles[style];

		const int16_t lineHeight = (props.size < 0 ? uint16_t(fontStyle.lineHeight) : uint16_t(props.size));
		const float relativeSize = (props.size < 0 ? 1 : (float)props.size / (float)fontStyle.lineHeight);

		const HALF hzero = XMConvertFloatToHalf(0.0f);
		const HALF hone = XMConvertFloatToHalf(1.0f);

		int16_t line = 0;
		int16_t pos = 0;
		for (size_t i = 0; i < text.length(); ++i)
		{
			const int code = text[i];

			if (fontStyle.lookup.count(code) == 0)
			{
				// glyph not packed yet, so add to pending list:
				pendingGlyphs.insert(glyphhash(code, style));
				continue;
			}

			if (code == '\n')
			{
				line += lineHeight + int(props.spacingY * relativeSize);
				pos = 0;
			}
			else if (code == ' ')
			{
				pos += int((WHITESPACE_SIZE + props.spacingX) * relativeSize);
			}
			else if (code == '\t')
			{
				pos += int(((WHITESPACE_SIZE + props.spacingX) * TAB_WHITESPACECOUNT) * relativeSize);
			}
			else
			{
				const wiFontStyle::Glyph& glyph = fontStyle.lookup.at(code);
				const int16_t glyphWidth = int16_t(glyph.width * relativeSize);
				const int16_t glyphHeight = int16_t(glyph.height * relativeSize);
				const int16_t glyphOffsetX = int16_t(glyph.x * relativeSize);
				const int16_t glyphOffsetY = int16_t(glyph.y * relativeSize);

				const size_t vertexID = quadCount * 4;

				const int16_t left = pos + glyphOffsetX;
				const int16_t right = left + glyphWidth;
				const int16_t top = line + glyphOffsetY;
				const int16_t bottom = top + glyphHeight;

				vertexList[vertexID + 0].Pos.x = left;
				vertexList[vertexID + 0].Pos.y = top;
				vertexList[vertexID + 1].Pos.x = right;
				vertexList[vertexID + 1].Pos.y = top;
				vertexList[vertexID + 2].Pos.x = left;
				vertexList[vertexID + 2].Pos.y = bottom;
				vertexList[vertexID + 3].Pos.x = right;
				vertexList[vertexID + 3].Pos.y = bottom;

				vertexList[vertexID + 0].Tex.x = glyph.tc_left;
				vertexList[vertexID + 0].Tex.y = glyph.tc_top;
				vertexList[vertexID + 1].Tex.x = glyph.tc_right;
				vertexList[vertexID + 1].Tex.y = glyph.tc_top;
				vertexList[vertexID + 2].Tex.x = glyph.tc_left;
				vertexList[vertexID + 2].Tex.y = glyph.tc_bottom;
				vertexList[vertexID + 3].Tex.x = glyph.tc_right;
				vertexList[vertexID + 3].Tex.y = glyph.tc_bottom;

				pos += glyphWidth + props.spacingX;

				//// add kerning
				//if (i > 0 && i < text.length() - 1)
				//{
				//	const wchar_t next_character = text[i + 1];
				//	if (next_character != ' ' && next_character != '\t')
				//	{
				//		int kern;
				//		kern = stbtt_GetCodepointKernAdvance(&fontStyle.fontInfo, code, next_character);
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
		AddFontStyle(FONTPATH + "arial.ttf", 16);
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

	{
		GPUBufferDesc bd;
		bd.Usage = USAGE_DYNAMIC;
		bd.ByteWidth = 256 * 1024; // just allocate 256KB to font renderer ring buffer..
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = CPU_ACCESS_WRITE;

		for (int i = 0; i < GRAPHICSTHREAD_COUNT; ++i)
		{
			HRESULT hr = device->CreateBuffer(&bd, nullptr, &vertexBuffers[i]);
			assert(SUCCEEDED(hr));
		}
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
	samplerDesc.AddressU = TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = TEXTURE_ADDRESS_BORDER;
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
	initialized.store(true);
}
void wiFont::CleanUp()
{
	fontStyles.clear();
	SAFE_DELETE(texture);
	SAFE_DELETE(vertexShader);
	SAFE_DELETE(pixelShader);
}

void wiFont::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R16G16_SINT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, FORMAT_R16G16_FLOAT, 0, APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	vertexShader = static_cast<VertexShader*>(wiResourceManager::GetShaderManager().add(path + "fontVS.cso", wiResourceManager::VERTEXSHADER));
	
	vertexLayout = new VertexLayout;
	wiRenderer::GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShader->code, vertexLayout);


	pixelShader = static_cast<PixelShader*>(wiResourceManager::GetShaderManager().add(path + "fontPS.cso", wiResourceManager::PIXELSHADER));


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


	// If there are pending glyphs, render them and repack the atlas:
	if (!pendingGlyphs.empty())
	{
		for (int64_t hash : pendingGlyphs)
		{
			int code = codefromhash(hash);
			int style = stylefromhash(hash);
			wiFontStyle& fontStyle = *fontStyles[style];

			// get bounding box for character (may be offset to account for chars that dip above or below the line
			int left, top, right, bottom;
			stbtt_GetCodepointBitmapBox(&fontStyle.fontInfo, code, fontStyle.fontScaling, fontStyle.fontScaling, &left, &top, &right, &bottom);

			wiFontStyle::Glyph& glyph = fontStyle.lookup[code];
			glyph.x = left;
			glyph.y = top + int(fontStyle.ascent);
			glyph.width = right - left;
			glyph.height = bottom - top;

			rects[hash] = rect_ltrb(left, top, right, bottom);
		}
		pendingGlyphs.clear();


		vector<rect_xywhf*> out_rects(rects.size());
		{
			int i = 0;
			for (auto& it : rects)
			{
				out_rects[i] = &it.second;
				i++;
			}
		}

		std::vector<bin> bins;
		if (pack(out_rects.data(), (int)rects.size(), 512, bins))
		{
			assert(bins.size() == 1 && "The regions won't fit into the texture!");

			const int bitmapWidth = bins[0].size.w;
			const int bitmapHeight = bins[0].size.h;
			const float inv_width = 1.0f / bitmapWidth;
			const float inv_height = 1.0f / bitmapHeight;

			vector<uint8_t> bitmap(bitmapWidth * bitmapHeight);
			std::fill(bitmap.begin(), bitmap.end(), 0);

			for (auto it : rects)
			{
				int64_t hash = it.first;
				wchar_t code = codefromhash(hash);
				int style = stylefromhash(hash);
				wiFontStyle& fontStyle = *fontStyles[style];
				rect_xywhf& rect = it.second;

				// render character (stride and offset is important here)
				int byteOffset = rect.x + (rect.y * bitmapWidth);
				stbtt_MakeCodepointBitmap(&fontStyle.fontInfo, bitmap.data() + byteOffset, rect.w, rect.h, bitmapWidth, fontStyle.fontScaling, fontStyle.fontScaling, code);

				float tc_left = float(rect.x);
				float tc_right = tc_left + float(rect.w);
				float tc_top = float(rect.y);
				float tc_bottom = tc_top + float(rect.h);

				tc_left *= inv_width;
				tc_right *= inv_width;
				tc_top *= inv_height;
				tc_bottom *= inv_height;

				wiFontStyle::Glyph& glyph = fontStyle.lookup[code];
				glyph.tc_left = XMConvertFloatToHalf(tc_left);
				glyph.tc_right = XMConvertFloatToHalf(tc_right);
				glyph.tc_top = XMConvertFloatToHalf(tc_top);
				glyph.tc_bottom = XMConvertFloatToHalf(tc_bottom);

			}

			HRESULT hr = wiTextureHelper::CreateTexture(texture, bitmap.data(), bitmapWidth, bitmapHeight, 1, FORMAT_R8_UNORM);
			assert(SUCCEEDED(hr));
		}
	}


	device->BindResource(PS, texture, TEXSLOT_FONTATLAS, threadID);
}
Texture2D* wiFont::GetAtlas()
{
	return texture;
}


void wiFont::Draw(GRAPHICSTHREAD threadID)
{
	if (!initialized.load() || text.length() <= 0)
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
	volatile FontVertex* textBuffer = (volatile FontVertex*)device->AllocateFromRingBuffer(&vertexBuffers[threadID], sizeof(FontVertex) * text.length() * 4, vboffset, threadID);
	if (textBuffer == nullptr)
	{
		return;
	}
	const int quadCount = ModifyGeo(textBuffer, text, newProps, style);
	device->InvalidateBufferAccess(&vertexBuffers[threadID], threadID);

	device->EventBegin("Font", threadID);

	device->BindGraphicsPSO(PSO, threadID);
	device->BindSampler(PS, &sampler, SSLOT_ONDEMAND1, threadID);

	GPUBuffer* vbs[] = {
		&vertexBuffers[threadID],
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
	const wiFontStyle& fontStyle = *fontStyles[style];
	const float relativeSize = (props.size < 0 ? 1.0f : (float)props.size / (float)fontStyle.lineHeight);
	int maxWidth = 0;
	int currentLineWidth = 0;
	for (size_t i = 0; i < text.length(); ++i)
	{
		const int code = text[i];

		if (fontStyle.lookup.count(code) == 0)
		{
			// glyph not packed yet, so add to pending list:
			pendingGlyphs.insert(glyphhash(code, style));
			continue;
		}

		if (code == '\n')
		{
			currentLineWidth = 0;
		}
		else if (code == ' ')
		{
			currentLineWidth += int((WHITESPACE_SIZE + props.spacingX) * relativeSize);
		}
		else if (code == '\t')
		{
			currentLineWidth += int(((WHITESPACE_SIZE + props.spacingX) * TAB_WHITESPACECOUNT) * relativeSize);
		}
		else
		{
			int characterWidth = (int)(fontStyle.lookup.at(code).width * relativeSize);
			currentLineWidth += characterWidth + int(props.spacingX * relativeSize);
		}
		maxWidth = max(maxWidth, currentLineWidth);
	}

	return maxWidth;
}
int wiFont::textHeight()
{
	const wiFontStyle& fontStyle = *fontStyles[style];
	const float relativeSize = (props.size < 0 ? 1.0f : (float)props.size / (float)fontStyle.lineHeight);
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

	const int lineHeight = (props.size < 0 ? fontStyle.lineHeight : props.size);
	return lines * (lineHeight + int(props.spacingY * relativeSize));
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

int wiFont::AddFontStyle(const string& fontName, int height)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		const wiFontStyle& fontStyle = *fontStyles[i];
		if (!fontStyle.name.compare(fontName) && fontStyle.lineHeight == height)
		{
			return int(i);
		}
	}
	fontStyles.push_back(new wiFontStyle(fontName, height));
	return int(fontStyles.size() - 1);
}
int wiFont::GetFontStyle(const string& fontName, int height)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		const wiFontStyle& fontStyle = *fontStyles[i];
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
