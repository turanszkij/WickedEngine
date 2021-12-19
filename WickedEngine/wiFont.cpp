#include "wiFont.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "shaders/ShaderInterop_Font.h"
#include "wiBacklog.h"
#include "wiTextureHelper.h"
#include "wiRectPacker.h"
#include "wiSpinLock.h"
#include "wiPlatform.h"
#include "wiEventHandler.h"
#include "wiTimer.h"
#include "wiUnorderedMap.h"
#include "wiUnorderedSet.h"
#include "wiVector.h"

#include "Utility/arial.h"
#include "Utility/stb_truetype.h"

#include <fstream>

using namespace wi::graphics;
using namespace wi::rectpacker;

namespace wi::font
{
#define WHITESPACE_SIZE ((float(params.size) + params.spacingX) * params.scaling * 0.25f)
#define TAB_SIZE (WHITESPACE_SIZE * 4)
#define LINEBREAK_SIZE ((float(params.size) + params.spacingY) * params.scaling)

	namespace font_internal
	{
		BlendState			blendState;
		RasterizerState		rasterizerState;
		DepthStencilState	depthStencilState;

		Shader				vertexShader;
		Shader				pixelShader;
		PipelineState		PSO;

		wi::Canvas canvases[COMMANDLIST_COUNT];

		Texture texture;

		struct Glyph
		{
			float x;
			float y;
			float width;
			float height;
			float tc_left;
			float tc_right;
			float tc_top;
			float tc_bottom;
		};
		wi::unordered_map<int32_t, Glyph> glyph_lookup;
		wi::unordered_map<int32_t, rect_xywh> rect_lookup;
		// pack glyph identifiers to a 32-bit hash:
		//	height:	10 bits	(height supported: 0 - 1023)
		//	style:	6 bits	(number of font styles supported: 0 - 63)
		//	code:	16 bits (character code range supported: 0 - 65535)
		constexpr int32_t glyphhash(int code, int style, int height) { return ((code & 0xFFFF) << 16) | ((style & 0x3F) << 10) | (height & 0x3FF); }
		constexpr int codefromhash(int64_t hash) { return int((hash >> 16) & 0xFFFF); }
		constexpr int stylefromhash(int64_t hash) { return int((hash >> 10) & 0x3F); }
		constexpr int heightfromhash(int64_t hash) { return int((hash >> 0) & 0x3FF); }
		wi::unordered_set<int32_t> pendingGlyphs;
		wi::SpinLock glyphLock;

		struct FontStyle
		{
			std::string name;
			wi::vector<uint8_t> fontBuffer; // only used if loaded from file, need to keep alive
			stbtt_fontinfo fontInfo;
			int ascent, descent, lineGap;
			void Create(const std::string& newName, const uint8_t* data, size_t size)
			{
				name = newName;
				int offset = stbtt_GetFontOffsetForIndex(data, 0);

				if (!stbtt_InitFont(&fontInfo, data, offset))
				{
					wi::backlog::post("Failed to load font: " + name + " (file was unrecognized, it must be a .ttf file)");
				}

				stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
			}
			void Create(const std::string& newName)
			{
				if (wi::helper::FileRead(newName, fontBuffer))
				{
					Create(newName, fontBuffer.data(), fontBuffer.size());
				}
				else
				{
					wi::backlog::post("Failed to load font: " + name + " (file could not be opened)");
				}
			}
		};
		wi::vector<FontStyle> fontStyles;

		template<typename T>
		uint32_t WriteVertices(FontVertex* vertexList, const T* text, Params params)
		{
			const FontStyle& fontStyle = fontStyles[params.style];
			const float fontScale = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, (float)params.size);

			uint32_t quadCount = 0;
			float line = 0;
			float pos = 0;
			float pos_last_letter = 0;
			size_t last_word_begin = 0;
			bool start_new_word = false;

			auto word_wrap = [&] {
				start_new_word = true;
				if (last_word_begin > 0 && params.h_wrap >= 0 && pos >= params.h_wrap - 1)
				{
					// Word ended and wrap detected, push down last word by one line:
					float word_offset = vertexList[last_word_begin].pos.x; // possibly uncached memory read?
					for (size_t i = last_word_begin; i < quadCount * 4; ++i)
					{
						vertexList[i].pos.x -= word_offset; // possibly uncached memory read?
						vertexList[i].pos.y += LINEBREAK_SIZE; // possibly uncached memory read?
					}
					line += LINEBREAK_SIZE;
					pos -= word_offset;
				}
			};

			int code_prev = 0;
			size_t i = 0;
			while (text[i] != 0)
			{
				T character = text[i++];
				int code = (int)character;
				const int32_t hash = glyphhash(code, params.style, params.size);

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
					word_wrap();
					line += LINEBREAK_SIZE;
					pos = 0;
					code_prev = 0;
				}
				else if (code == ' ')
				{
					word_wrap();
					pos += WHITESPACE_SIZE;
					code_prev = 0;
				}
				else if (code == '\t')
				{
					word_wrap();
					pos += TAB_SIZE;
					code_prev = 0;
				}
				else
				{
					const Glyph& glyph = glyph_lookup.at(hash);
					const float glyphWidth = glyph.width * params.scaling;
					const float glyphHeight = glyph.height * params.scaling;
					const float glyphOffsetX = glyph.x * params.scaling;
					const float glyphOffsetY = glyph.y * params.scaling;

					const size_t vertexID = size_t(quadCount) * 4;

					if (start_new_word)
					{
						last_word_begin = vertexID;
					}
					start_new_word = false;

					if (code_prev != 0)
					{
						int kern = stbtt_GetCodepointKernAdvance(&fontStyle.fontInfo, code_prev, code);
						pos += kern * fontScale;
					}
					code_prev = code;

					const float left = pos + glyphOffsetX;
					const float right = left + glyphWidth;
					const float top = line + glyphOffsetY;
					const float bottom = top + glyphHeight;

					vertexList[vertexID + 0].pos = float2(left, top);
					vertexList[vertexID + 1].pos = float2(right, top);
					vertexList[vertexID + 2].pos = float2(left, bottom);
					vertexList[vertexID + 3].pos = float2(right, bottom);

					vertexList[vertexID + 0].uv = float2(glyph.tc_left, glyph.tc_top);
					vertexList[vertexID + 1].uv = float2(glyph.tc_right, glyph.tc_top);
					vertexList[vertexID + 2].uv = float2(glyph.tc_left, glyph.tc_bottom);
					vertexList[vertexID + 3].uv = float2(glyph.tc_right, glyph.tc_bottom);

					pos += glyph.width * params.scaling + params.spacingX;
					pos_last_letter = pos;

					quadCount++;
				}

			}

			word_wrap();

			return quadCount;
		}

	}
	using namespace font_internal;

	void LoadShaders()
	{
		wi::renderer::LoadShader(ShaderStage::VS, vertexShader, "fontVS.cso");
		wi::renderer::LoadShader(ShaderStage::PS, pixelShader, "fontPS.cso");

		PipelineStateDesc desc;
		desc.vs = &vertexShader;
		desc.ps = &pixelShader;
		desc.bs = &blendState;
		desc.dss = &depthStencilState;
		desc.rs = &rasterizerState;
		desc.pt = PrimitiveTopology::TRIANGLESTRIP;
		wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO);
	}
	void Initialize()
	{
		wi::Timer timer;

		// add default font if there is none yet:
		if (fontStyles.empty())
		{
			AddFontStyle("arial", arial, sizeof(arial));
		}

		GraphicsDevice* device = wi::graphics::GetDevice();

		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::FRONT;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rasterizerState = rs;

		BlendState bd;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ONE;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendState = bd;

		DepthStencilState dsd;
		dsd.depth_enable = false;
		dsd.stencil_enable = false;
		depthStencilState = dsd;

		static wi::eventhandler::Handle handle1 = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wi::backlog::post("wi::font Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	void UpdatePendingGlyphs()
	{
		glyphLock.lock();

		// If there are pending glyphs, render them and repack the atlas:
		if (!pendingGlyphs.empty())
		{
			// Pad the glyph rects in the atlas to avoid bleeding from nearby texels:
			const int borderPadding = 1;

			// Font resolution is upscaled to make it sharper:
			const float upscaling = 2.0f;

			for (int32_t hash : pendingGlyphs)
			{
				const int code = codefromhash(hash);
				const int style = stylefromhash(hash);
				const float height = (float)heightfromhash(hash) * upscaling;
				FontStyle& fontStyle = fontStyles[style];

				float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, height);

				// get bounding box for character (may be offset to account for chars that dip above or below the line
				int left, top, right, bottom;
				stbtt_GetCodepointBitmapBox(&fontStyle.fontInfo, code, fontScaling, fontScaling, &left, &top, &right, &bottom);

				// Glyph dimensions are calculated without padding:
				Glyph& glyph = glyph_lookup[hash];
				glyph.x = float(left);
				glyph.y = float(top) + float(fontStyle.ascent) * fontScaling;
				glyph.width = float(right - left);
				glyph.height = float(bottom - top);

				// Remove dpi upscaling:
				glyph.x = glyph.x / upscaling;
				glyph.y = glyph.y / upscaling;
				glyph.width = glyph.width / upscaling;
				glyph.height = glyph.height / upscaling;

				// Add padding to the rectangle that will be packed in the atlas:
				right += borderPadding * 2;
				bottom += borderPadding * 2;
				rect_lookup[hash] = rect_ltrb(left, top, right, bottom);
			}
			pendingGlyphs.clear();

			// This reference array will be used for packing:
			wi::vector<rect_xywh*> out_rects;
			out_rects.reserve(rect_lookup.size());
			for (auto& it : rect_lookup)
			{
				out_rects.push_back(&it.second);
			}

			// Perform packing and process the result if successful:
			wi::vector<bin> bins;
			if (pack(out_rects.data(), (int)out_rects.size(), 4096, bins))
			{
				assert(bins.size() == 1 && "The regions won't fit into one texture!");

				// Retrieve texture atlas dimensions:
				const int bitmapWidth = bins[0].size.w;
				const int bitmapHeight = bins[0].size.h;
				const float inv_width = 1.0f / bitmapWidth;
				const float inv_height = 1.0f / bitmapHeight;

				// Create the CPU-side texture atlas and fill with transparency (0):
				wi::vector<uint8_t> bitmap(size_t(bitmapWidth) * size_t(bitmapHeight));
				std::fill(bitmap.begin(), bitmap.end(), 0);

				// Iterate all packed glyph rectangles:
				for (auto it : rect_lookup)
				{
					const int32_t hash = it.first;
					const wchar_t code = codefromhash(hash);
					const int style = stylefromhash(hash);
					const float height = (float)heightfromhash(hash) * upscaling;
					const FontStyle& fontStyle = fontStyles[style];
					rect_xywh& rect = it.second;
					Glyph& glyph = glyph_lookup[hash];

					// Remove border padding from the packed rectangle (we don't want to touch the border, it should stay transparent):
					rect.x += borderPadding;
					rect.y += borderPadding;
					rect.w -= borderPadding * 2;
					rect.h -= borderPadding * 2;

					float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, height);

					// Render the glyph inside the CPU-side atlas:
					int byteOffset = rect.x + (rect.y * bitmapWidth);
					stbtt_MakeCodepointBitmap(&fontStyle.fontInfo, bitmap.data() + byteOffset, rect.w, rect.h, bitmapWidth, fontScaling, fontScaling, code);

					// Compute texture coordinates for the glyph:
					glyph.tc_left = float(rect.x);
					glyph.tc_right = glyph.tc_left + float(rect.w);
					glyph.tc_top = float(rect.y);
					glyph.tc_bottom = glyph.tc_top + float(rect.h);

					glyph.tc_left *= inv_width;
					glyph.tc_right *= inv_width;
					glyph.tc_top *= inv_height;
					glyph.tc_bottom *= inv_height;
				}

				// Upload the CPU-side texture atlas bitmap to the GPU:
				wi::texturehelper::CreateTexture(texture, bitmap.data(), bitmapWidth, bitmapHeight, Format::R8_UNORM);
			}
		}

		glyphLock.unlock();
	}
	const Texture* GetAtlas()
	{
		return &texture;
	}
	int AddFontStyle(const std::string& fontName)
	{
		for (size_t i = 0; i < fontStyles.size(); i++)
		{
			const FontStyle& fontStyle = fontStyles[i];
			if (!fontStyle.name.compare(fontName))
			{
				return int(i);
			}
		}
		fontStyles.emplace_back();
		fontStyles.back().Create(fontName);
		return int(fontStyles.size() - 1);
	}
	int AddFontStyle(const std::string& fontName, const uint8_t* data, size_t size)
	{
		for (size_t i = 0; i < fontStyles.size(); i++)
		{
			const FontStyle& fontStyle = fontStyles[i];
			if (!fontStyle.name.compare(fontName))
			{
				return int(i);
			}
		}
		fontStyles.emplace_back();
		fontStyles.back().Create(fontName, data, size);
		return int(fontStyles.size() - 1);
	}


	template<typename T>
	float TextWidth_internal(const T* text, const Params& params)
	{
		if (params.style >= (int)fontStyles.size())
		{
			return 0;
		}

		// TODO: account for word wrap
		float maxWidth = 0;
		float currentLineWidth = 0;
		size_t i = 0;
		while (text[i] != 0)
		{
			int code = (int)text[i++];
			const int32_t hash = glyphhash(code, params.style, params.size);

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
				currentLineWidth += glyph.width + float(params.spacingX) * params.scaling;
			}
			maxWidth = std::max(maxWidth, currentLineWidth);
		}

		return maxWidth;
	}

	template<typename T>
	float TextHeight_internal(const T* text, const Params& params)
	{
		if (params.style >= (int)fontStyles.size())
		{
			return 0;
		}

		// TODO: account for word wrap
		float height = LINEBREAK_SIZE;
		size_t i = 0;
		while (text[i] != 0)
		{
			int code = (int)text[i++];
			if (code == '\n')
			{
				height += LINEBREAK_SIZE;
			}
		}

		return height;
	}

	template<typename T>
	void Draw_internal(const T* text, size_t text_length, const Params& params, CommandList cmd)
	{
		if (text_length <= 0)
		{
			return;
		}

		Params newProps = params;

		if (params.h_align == WIFALIGN_CENTER)
			newProps.posX -= TextWidth_internal(text, newProps) / 2;
		else if (params.h_align == WIFALIGN_RIGHT)
			newProps.posX -= TextWidth_internal(text, newProps);
		if (params.v_align == WIFALIGN_CENTER)
			newProps.posY -= TextHeight_internal(text, newProps) / 2;
		else if (params.v_align == WIFALIGN_BOTTOM)
			newProps.posY -= TextHeight_internal(text, newProps);

		GraphicsDevice* device = wi::graphics::GetDevice();

		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(FontVertex) * text_length * 4, cmd);
		if (!mem.IsValid())
		{
			return;
		}
		FontVertex* textBuffer = (FontVertex*)mem.data;
		const uint32_t quadCount = WriteVertices(textBuffer, text, newProps);

		if (quadCount > 0)
		{
			device->EventBegin("Font", cmd);

			device->BindPipelineState(&PSO, cmd);

			FontConstants font;
			FontPushConstants font_push;
			font_push.buffer_index = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
			font_push.buffer_offset = (uint32_t)mem.offset;
			font_push.texture_index = device->GetDescriptorIndex(&texture, SubresourceType::SRV);

			const wi::Canvas& canvas = canvases[cmd];
			// Asserts will check that a proper canvas was set for this cmd with wi::image::SetCanvas()
			//	The canvas must be set to have dpi aware rendering
			assert(canvas.width > 0);
			assert(canvas.height > 0);
			assert(canvas.dpi > 0);
			const XMMATRIX Projection = canvas.GetProjection();

			if (newProps.shadowColor.getA() > 0)
			{
				// font shadow render:
				XMStoreFloat4x4(&font.transform,
					XMMatrixTranslation((float)newProps.posX + 1, (float)newProps.posY + 1, 0)
					* Projection
				);
				device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

				font_push.color = newProps.shadowColor.rgba;
				device->PushConstants(&font_push, sizeof(font_push), cmd);

				device->DrawInstanced(4, quadCount, 0, 0, cmd);
			}

			// font base render:
			XMStoreFloat4x4(&font.transform,
				XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
				* Projection
			);
			device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

			font_push.color = newProps.color.rgba;
			device->PushConstants(&font_push, sizeof(font_push), cmd);

			device->DrawInstanced(4, quadCount, 0, 0, cmd);

			device->EventEnd(cmd);
		}

		UpdatePendingGlyphs();
	}

	void SetCanvas(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
	{
		canvases[cmd] = canvas;
	}

	void Draw(const char* text, const Params& params, CommandList cmd)
	{
		size_t text_length = strlen(text);
		if (text_length == 0)
		{
			return;
		}
		Draw_internal(text, text_length, params, cmd);
	}
	void Draw(const wchar_t* text, const Params& params, CommandList cmd)
	{
		size_t text_length = wcslen(text);
		if (text_length == 0)
		{
			return;
		}
		Draw_internal(text, text_length, params, cmd);
	}
	void Draw(const std::string& text, const Params& params, CommandList cmd)
	{
		Draw_internal(text.c_str(), text.length(), params, cmd);
	}
	void Draw(const std::wstring& text, const Params& params, CommandList cmd)
	{
		Draw_internal(text.c_str(), text.length(), params, cmd);
	}

	float TextWidth(const char* text, const Params& params)
	{
		return TextWidth_internal(text, params);
	}
	float TextWidth(const wchar_t* text, const Params& params)
	{
		return TextWidth_internal(text, params);
	}
	float TextWidth(const std::string& text, const Params& params)
	{
		return TextWidth_internal(text.c_str(), params);
	}
	float TextWidth(const std::wstring& text, const Params& params)
	{
		return TextWidth_internal(text.c_str(), params);
	}

	float TextHeight(const char* text, const Params& params)
	{
		return TextHeight_internal(text, params);
	}
	float TextHeight(const wchar_t* text, const Params& params)
	{
		return TextHeight_internal(text, params);
	}
	float TextHeight(const std::string& text, const Params& params)
	{
		return TextHeight_internal(text.c_str(), params);
	}
	float TextHeight(const std::wstring& text, const Params& params)
	{
		return TextHeight_internal(text.c_str(), params);
	}

}
