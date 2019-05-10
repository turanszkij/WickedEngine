#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "ShaderInterop_Font.h"
#include "wiBackLog.h"
#include "wiTextureHelper.h"
#include "wiRectPacker.h"
#include "wiSpinLock.h"

#include "Utility/stb_truetype.h"

#include <fstream>
#include <sstream>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace wiGraphics;
using namespace wiRectPacker;

#define MAX_TEXT 10000
#define WHITESPACE_SIZE (int((params.size + params.spacingX) * params.scaling * 0.3f))
#define TAB_SIZE (WHITESPACE_SIZE * 4)
#define LINEBREAK_SIZE (int((params.size + params.spacingY) * params.scaling))

namespace wiFont_Internal
{
	std::string			FONTPATH = "fonts/";
	GPUBuffer			indexBuffer;
	GPUBuffer			constantBuffer;
	BlendState			blendState;
	RasterizerState		rasterizerState;
	DepthStencilState	depthStencilState;
	Sampler				sampler;

	VertexLayout		vertexLayout;
	const VertexShader	*vertexShader = nullptr;
	const PixelShader	*pixelShader = nullptr;
	GraphicsPSO			PSO;

	atomic_bool initialized = false;

	Texture2D texture;

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
	unordered_map<int32_t, Glyph> glyph_lookup;
	unordered_map<int32_t, rect_xywh> rect_lookup;
	// pack glyph identifiers to a 32-bit hash:
	//	height:	10 bits	(height supported: 0 - 1023)
	//	style:	6 bits	(number of font styles supported: 0 - 63)
	//	code:	16 bits (character code range supported: 0 - 65535)
	constexpr int32_t glyphhash(int code, int style, int height) { return ((code & 0xFFFF) << 16) | ((style & 0x3F) << 10) | (height & 0x3FF); }
	constexpr int codefromhash(int64_t hash) { return int((hash >> 16) & 0xFFFF); }
	constexpr int stylefromhash(int64_t hash) { return int((hash >> 10) & 0x3F); }
	constexpr int heightfromhash(int64_t hash) { return int((hash >> 0) & 0x3FF); }
	unordered_set<int32_t> pendingGlyphs;
	wiSpinLock glyphLock;

	struct wiFontStyle
	{
		string name;
		vector<uint8_t> fontBuffer;
		stbtt_fontinfo fontInfo;
		int ascent, descent, lineGap;
		wiFontStyle(const string& newName) : name(newName)
		{
			wiHelper::readByteData(newName, fontBuffer);

			int offset = stbtt_GetFontOffsetForIndex(fontBuffer.data(), 0);

			if (!stbtt_InitFont(&fontInfo, fontBuffer.data(), offset))
			{
				stringstream ss("");
				ss << "Failed to load font: " << name;
				wiHelper::messageBox(ss.str());
			}

			stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
		}
	};
	std::vector<wiFontStyle*> fontStyles;

	struct FontVertex
	{
		XMSHORT2 Pos;
		XMHALF2 Tex;
	};

	int WriteVertices(volatile FontVertex* vertexList, const std::wstring& text, wiFontParams params, int style)
	{
		int quadCount = 0;

		int16_t line = 0;
		int16_t pos = 0;
		for (auto& code : text)
		{
			const int32_t hash = glyphhash(code, style, params.size);

			if (glyph_lookup.count(hash) == 0)
			{
				// glyph not packed yet, so add to pending list:
				glyphLock.lock();
				pendingGlyphs.insert(hash);
				glyphLock.unlock();
				continue;
			}

			if (code == '\n')
			{
				line += LINEBREAK_SIZE;
				pos = 0;
			}
			else if (code == ' ')
			{
				pos += WHITESPACE_SIZE;
			}
			else if (code == '\t')
			{
				pos += TAB_SIZE;
			}
			else
			{
				const Glyph& glyph = glyph_lookup.at(hash);
				const int16_t glyphWidth = int16_t(glyph.width * params.scaling);
				const int16_t glyphHeight = int16_t(glyph.height * params.scaling);
				const int16_t glyphOffsetX = int16_t(glyph.x * params.scaling);
				const int16_t glyphOffsetY = int16_t(glyph.y * params.scaling);

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

				pos += int16_t((glyph.width + params.spacingX) * params.scaling);

				quadCount++;
			}

		}

		return quadCount;
	}

}
using namespace wiFont_Internal;


wiFont::wiFont(const std::string& text, wiFontParams params, int style) : params(params), style(style)
{
	SetText(text);
}
wiFont::wiFont(const std::wstring& text, wiFontParams params, int style) : params(params), style(style)
{
	SetText(text);
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
		AddFontStyle(FONTPATH + "arial.ttf");
	}

	GraphicsDevice* device = wiRenderer::GetDevice();

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
	samplerDesc.MaxLOD = FLT_MAX;
	device->CreateSamplerState(&samplerDesc, &sampler);

	LoadShaders();

	wiBackLog::post("wiFont Initialized");
	initialized.store(true);
}
void wiFont::CleanUp()
{
	fontStyles.clear();
}

void wiFont::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	VertexLayoutDesc layout[] =
	{
		{ "POSITION", 0, FORMAT_R16G16_SINT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, FORMAT_R16G16_FLOAT, 0, VertexLayoutDesc::APPEND_ALIGNED_ELEMENT, INPUT_PER_VERTEX_DATA, 0 },
	};
	vertexShader = static_cast<const VertexShader*>(wiResourceManager::GetShaderManager().add(path + "fontVS.cso", wiResourceManager::VERTEXSHADER));
	
	wiRenderer::GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), &vertexShader->code, &vertexLayout);


	pixelShader = static_cast<const PixelShader*>(wiResourceManager::GetShaderManager().add(path + "fontPS.cso", wiResourceManager::PIXELSHADER));


	GraphicsPSODesc desc;
	desc.vs = vertexShader;
	desc.ps = pixelShader;
	desc.il = &vertexLayout;
	desc.bs = &blendState;
	desc.rs = &rasterizerState;
	desc.dss = &depthStencilState;
	desc.numRTs = 1;
	desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
	wiRenderer::GetDevice()->CreateGraphicsPSO(&desc, &PSO);
}

void UpdatePendingGlyphs()
{
	// If there are pending glyphs, render them and repack the atlas:
	if (!pendingGlyphs.empty())
	{
		// Pad the glyph rects in the atlas to avoid bleeding from nearby texels:
		const int borderPadding = 1;

		for (int32_t hash : pendingGlyphs)
		{
			const int code = codefromhash(hash);
			const int style = stylefromhash(hash);
			const int height = heightfromhash(hash);
			wiFontStyle& fontStyle = *fontStyles[style];

			float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, float(height));

			// get bounding box for character (may be offset to account for chars that dip above or below the line
			int left, top, right, bottom;
			stbtt_GetCodepointBitmapBox(&fontStyle.fontInfo, code, fontScaling, fontScaling, &left, &top, &right, &bottom);

			// Glyph dimensions are calculated without padding:
			Glyph& glyph = glyph_lookup[hash];
			glyph.x = left;
			glyph.y = top + int(fontStyle.ascent * fontScaling);
			glyph.width = right - left;
			glyph.height = bottom - top;

			// Add padding to the rectangle that will be packed in the atlas:
			right += borderPadding * 2;
			bottom += borderPadding * 2;
			rect_lookup[hash] = rect_ltrb(left, top, right, bottom);
		}
		pendingGlyphs.clear();

		// This reference array will be used for packing:
		vector<rect_xywh*> out_rects;
		out_rects.reserve(rect_lookup.size());
		for (auto& it : rect_lookup)
		{
			out_rects.push_back(&it.second);
		}

		// Perform packing and process the result if successful:
		std::vector<bin> bins;
		if (pack(out_rects.data(), (int)out_rects.size(), 4096, bins))
		{
			assert(bins.size() == 1 && "The regions won't fit into one texture!");

			// Retrieve texture atlas dimensions:
			const int bitmapWidth = bins[0].size.w;
			const int bitmapHeight = bins[0].size.h;
			const float inv_width = 1.0f / bitmapWidth;
			const float inv_height = 1.0f / bitmapHeight;

			// Create the CPU-side texture atlas and fill with transparency (0):
			vector<uint8_t> bitmap(bitmapWidth * bitmapHeight);
			std::fill(bitmap.begin(), bitmap.end(), 0);

			// Iterate all packed glyph rectangles:
			for (auto it : rect_lookup)
			{
				const int32_t hash = it.first;
				const wchar_t code = codefromhash(hash);
				const int style = stylefromhash(hash);
				const int height = heightfromhash(hash);
				wiFontStyle& fontStyle = *fontStyles[style];
				rect_xywh& rect = it.second;
				Glyph& glyph = glyph_lookup[hash];

				// Remove border padding from the packed rectangle (we don't want to touch the border, it should stay transparent):
				rect.x += borderPadding;
				rect.y += borderPadding;
				rect.w -= borderPadding * 2;
				rect.h -= borderPadding * 2;

				float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, float(height));

				// Render the glyph inside the CPU-side atlas:
				int byteOffset = rect.x + (rect.y * bitmapWidth);
				stbtt_MakeCodepointBitmap(&fontStyle.fontInfo, bitmap.data() + byteOffset, rect.w, rect.h, bitmapWidth, fontScaling, fontScaling, code);

				// Compute texture coordinates for the glyph:
				float tc_left = float(rect.x);
				float tc_right = tc_left + float(rect.w);
				float tc_top = float(rect.y);
				float tc_bottom = tc_top + float(rect.h);

				tc_left *= inv_width;
				tc_right *= inv_width;
				tc_top *= inv_height;
				tc_bottom *= inv_height;

				glyph.tc_left = XMConvertFloatToHalf(tc_left);
				glyph.tc_right = XMConvertFloatToHalf(tc_right);
				glyph.tc_top = XMConvertFloatToHalf(tc_top);
				glyph.tc_bottom = XMConvertFloatToHalf(tc_bottom);

			}

			// Upload the CPU-side texture atlas bitmap to the GPU:
			HRESULT hr = wiTextureHelper::CreateTexture(texture, bitmap.data(), bitmapWidth, bitmapHeight, FORMAT_R8_UNORM);
			assert(SUCCEEDED(hr));
		}
	}
}
const Texture2D* wiFont::GetAtlas()
{
	return &texture;
}
std::string& wiFont::GetFontPath()
{
	return FONTPATH;
}
int wiFont::AddFontStyle(const string& fontName)
{
	for (size_t i = 0; i < fontStyles.size(); i++)
	{
		const wiFontStyle& fontStyle = *fontStyles[i];
		if (!fontStyle.name.compare(fontName))
		{
			return int(i);
		}
	}
	fontStyles.push_back(new wiFontStyle(fontName));
	return int(fontStyles.size() - 1);
}


void wiFont::Draw(GRAPHICSTHREAD threadID) const
{
	if (!initialized.load() || text.length() <= 0)
	{
		return;
	}

	wiFontParams newProps = params;

	if (params.h_align == WIFALIGN_CENTER)
		newProps.posX -= textWidth() / 2;
	else if (params.h_align == WIFALIGN_RIGHT)
		newProps.posX -= textWidth();
	if (params.v_align == WIFALIGN_CENTER)
		newProps.posY -= textHeight() / 2;
	else if (params.v_align == WIFALIGN_BOTTOM)
		newProps.posY -= textHeight();



	GraphicsDevice* device = wiRenderer::GetDevice();

	GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(FontVertex) * text.length() * 4, threadID);
	if (!mem.IsValid())
	{
		return;
	}
	volatile FontVertex* textBuffer = (volatile FontVertex*)mem.data;
	const int quadCount = WriteVertices(textBuffer, text, newProps, style);

	device->EventBegin("Font", threadID);

	device->BindGraphicsPSO(&PSO, threadID);

	device->BindConstantBuffer(VS, &constantBuffer, CB_GETBINDSLOT(FontCB), threadID);
	device->BindConstantBuffer(PS, &constantBuffer, CB_GETBINDSLOT(FontCB), threadID);
	device->BindResource(PS, &texture, TEXSLOT_FONTATLAS, threadID);
	device->BindSampler(PS, &sampler, SSLOT_ONDEMAND1, threadID);

	const GPUBuffer* vbs[] = {
		mem.buffer,
	};
	const UINT strides[] = {
		sizeof(FontVertex),
	};
	const UINT offsets[] = {
		mem.offset,
	};
	device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, offsets, threadID);

	assert(text.length() * 4 < 65536 && "The index buffer currently only supports so many characters!");
	device->BindIndexBuffer(&indexBuffer, INDEXFORMAT_16BIT, 0, threadID);

	FontCB cb;

	if (newProps.shadowColor.getA() > 0)
	{
		// font shadow render:
		XMStoreFloat4x4(&cb.g_xFont_Transform, XMMatrixTranspose(
			XMMatrixTranslation((float)newProps.posX + 1, (float)newProps.posY + 1, 0)
			* device->GetScreenProjection()
		));
		cb.g_xFont_Color = newProps.shadowColor.toFloat4();
		device->UpdateBuffer(&constantBuffer, &cb, threadID);

		device->DrawIndexed(quadCount * 6, 0, 0, threadID);
	}

	// font base render:
	XMStoreFloat4x4(&cb.g_xFont_Transform, XMMatrixTranspose(
		XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
		* device->GetScreenProjection()
	));
	cb.g_xFont_Color = newProps.color.toFloat4();
	device->UpdateBuffer(&constantBuffer, &cb, threadID);

	device->DrawIndexed(quadCount * 6, 0, 0, threadID);

	device->EventEnd(threadID);

	UpdatePendingGlyphs();
}


int wiFont::textWidth() const
{
	if (style >= (int)fontStyles.size())
	{
		return 0;
	}

	int maxWidth = 0;
	int currentLineWidth = 0;
	for (auto& code : text)
	{
		const int32_t hash = glyphhash(code, style, params.size);

		if (glyph_lookup.count(hash) == 0)
		{
			// glyph not packed yet, we just continue (it will be added if it is actually rendered)
			continue;
		}

		if (code == '\n')
		{
			currentLineWidth = 0;
		}
		else if (code == ' ')
		{
			currentLineWidth += WHITESPACE_SIZE;
		}
		else if (code == '\t')
		{
			currentLineWidth += TAB_SIZE;
		}
		else
		{
			const Glyph& glyph = glyph_lookup.at(hash);
			currentLineWidth += int((glyph.width + params.spacingX) * params.scaling);
		}
		maxWidth = std::max(maxWidth, currentLineWidth);
	}

	return maxWidth;
}
int wiFont::textHeight() const
{
	if (style >= (int)fontStyles.size())
	{
		return 0;
	}

	int height = LINEBREAK_SIZE;
	for(auto& code : text)
	{
		if (code == '\n')
		{
			height += LINEBREAK_SIZE;
		}
	}

	return height;
}


void wiFont::SetText(const string& text)
{
#ifdef WIN32
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
	wchar_t* wstr = new wchar_t[wchars_num];
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wstr, wchars_num);
	this->text = wstr;
	delete[] wstr;
#else
	this->text = wstring(text.begin(), text.end());
#endif
}
void wiFont::SetText(const wstring& text)
{
	this->text = text;
}
wstring wiFont::GetText() const
{
	return text;
}
string wiFont::GetTextA() const
{
	return string(text.begin(),text.end());
}
