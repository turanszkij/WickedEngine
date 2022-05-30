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
#include <mutex>

using namespace wi::graphics;
using namespace wi::rectpacker;

namespace wi::font
{
#define WHITESPACE_SIZE ((float(params.size) + params.spacingX) * params.scaling * 0.25f)
#define TAB_SIZE (WHITESPACE_SIZE * 4)
#define LINEBREAK_SIZE ((float(params.size) + params.spacingY) * params.scaling)

	namespace font_internal
	{
		static BlendState blendState;
		static RasterizerState rasterizerState;
		static DepthStencilState depthStencilState;

		static Shader vertexShader;
		static Shader pixelShader;
		static PipelineState PSO;

		static thread_local wi::Canvas canvas;

		static Texture texture;

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
			float off_x;
			float off_y;
			float off_w;
			float off_h;
		};
		static wi::unordered_map<int32_t, Glyph> glyph_lookup;
		static wi::unordered_map<int32_t, rect_xywh> rect_lookup;
		struct SDF
		{
			static constexpr int padding = 5;
			static constexpr unsigned char onedge_value = 180;
			static constexpr float pixel_dist_scale = float(onedge_value) / float(padding);
			int width;
			int height;
			int xoff;
			int yoff;
			wi::vector<uint8_t> bitmap;
		};
		static wi::unordered_map<int32_t, SDF> sdf_lookup;
		// pack glyph identifiers to a 32-bit hash:
		//	height:	10 bits	(height supported: 0 - 1023)
		//	style:	6 bits	(number of font styles supported: 0 - 63)
		//	code:	16 bits (character code range supported: 0 - 65535)
		constexpr int32_t glyphhash(int code, int style, int height) { return ((code & 0xFFFF) << 16) | ((style & 0x3F) << 10) | (height & 0x3FF); }
		constexpr int codefromhash(int64_t hash) { return int((hash >> 16) & 0xFFFF); }
		constexpr int stylefromhash(int64_t hash) { return int((hash >> 10) & 0x3F); }
		constexpr int heightfromhash(int64_t hash) { return int((hash >> 0) & 0x3FF); }
		static wi::unordered_set<int32_t> pendingGlyphs;
		static wi::SpinLock glyphLock;

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
		static wi::vector<FontStyle> fontStyles;

		struct Cursor
		{
			XMFLOAT2 pos = {};
			XMFLOAT2 size = {};
			uint32_t quadCount = 0;
			size_t last_word_begin = 0;
			bool start_new_word = false;
		};

		static thread_local wi::vector<FontVertex> vertexList;

		template<typename T>
		Cursor ParseText(const T* text, size_t text_length, Params params)
		{
			Cursor cursor;
			const FontStyle& fontStyle = fontStyles[params.style];
			const float fontScale = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, (float)params.size);
			vertexList.clear();

			auto word_wrap = [&] {
				cursor.start_new_word = true;
				if (cursor.last_word_begin > 0 && params.h_wrap >= 0 && cursor.pos.x >= params.h_wrap - 1)
				{
					// Word ended and wrap detected, push down last word by one line:
					float word_offset = vertexList[cursor.last_word_begin].pos.x;
					for (size_t i = cursor.last_word_begin; i < cursor.quadCount * 4; ++i)
					{
						vertexList[i].pos.x -= word_offset;
						vertexList[i].pos.y += LINEBREAK_SIZE;
					}
					cursor.pos.x -= word_offset;
					cursor.pos.y += LINEBREAK_SIZE;
				}
			};

			int code_prev = 0;
			for (size_t i = 0; i < text_length; ++i)
			{
				T character = text[i];
				int code = (int)character;
				const int32_t hash = glyphhash(code, params.style, params.size);

				if (glyph_lookup.count(hash) == 0)
				{
					// glyph not packed yet, so add to pending list:
					std::scoped_lock locker(glyphLock);
					pendingGlyphs.insert(hash);
					continue;
				}

				if (code == '\n')
				{
					word_wrap();
					cursor.pos.x = 0;
					cursor.pos.y += LINEBREAK_SIZE;
					code_prev = 0;
				}
				else if (code == ' ')
				{
					word_wrap();
					cursor.pos.x += WHITESPACE_SIZE;
					code_prev = 0;
				}
				else if (code == '\t')
				{
					word_wrap();
					cursor.pos.x += TAB_SIZE;
					code_prev = 0;
				}
				else
				{
					const Glyph& glyph = glyph_lookup.at(hash);
					const float glyphWidth = glyph.width * params.scaling;
					const float glyphHeight = glyph.height * params.scaling;
					const float glyphOffsetX = (glyph.x + glyph.off_x) * params.scaling;
					const float glyphOffsetY = glyph.y * params.scaling;

					const size_t vertexID = size_t(cursor.quadCount) * 4;
					vertexList.resize(vertexID + 4);
					cursor.quadCount++;

					if (cursor.start_new_word)
					{
						cursor.last_word_begin = vertexID;
					}
					cursor.start_new_word = false;

					if (code_prev != 0)
					{
						int kern = stbtt_GetCodepointKernAdvance(&fontStyle.fontInfo, code_prev, code);
						cursor.pos.x += kern * fontScale;
					}
					code_prev = code;

					const float left = cursor.pos.x + glyphOffsetX;
					const float right = left + glyphWidth;
					const float top = cursor.pos.y + glyphOffsetY;
					const float bottom = top + glyphHeight;

					vertexList[vertexID + 0].pos = float2(left, top);
					vertexList[vertexID + 1].pos = float2(right, top);
					vertexList[vertexID + 2].pos = float2(left, bottom);
					vertexList[vertexID + 3].pos = float2(right, bottom);

					vertexList[vertexID + 0].uv = float2(glyph.tc_left, glyph.tc_top);
					vertexList[vertexID + 1].uv = float2(glyph.tc_right, glyph.tc_top);
					vertexList[vertexID + 2].uv = float2(glyph.tc_left, glyph.tc_bottom);
					vertexList[vertexID + 3].uv = float2(glyph.tc_right, glyph.tc_bottom);

					cursor.pos.x += glyph.off_w * params.scaling + params.spacingX;
				}

				cursor.size.x = std::max(cursor.size.x, cursor.pos.x);
				cursor.size.y = std::max(cursor.size.y, cursor.pos.y + LINEBREAK_SIZE);
			}

			word_wrap();

			return cursor;
		}
		void CommitText(void* vertexList_GPU)
		{
			std::memcpy(vertexList_GPU, vertexList.data(), sizeof(FontVertex) * vertexList.size());
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

	void UpdateAtlas()
	{
		std::scoped_lock locker(glyphLock);

		// If there are pending glyphs, render them and repack the atlas:
		if (!pendingGlyphs.empty())
		{
			for (int32_t hash : pendingGlyphs)
			{
				const int code = codefromhash(hash);
				const int style = stylefromhash(hash);
				const float height = (float)heightfromhash(hash);
				FontStyle& fontStyle = fontStyles[style];

				float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, height);

				SDF& sdf = sdf_lookup[hash];
				sdf.width = 0;
				sdf.height = 0;
				sdf.xoff = 0;
				sdf.yoff = 0;
				unsigned char* bitmap = stbtt_GetCodepointSDF(&fontStyle.fontInfo, fontScaling, code, sdf.padding, sdf.onedge_value, sdf.pixel_dist_scale, &sdf.width, &sdf.height, &sdf.xoff, &sdf.yoff);
				sdf.bitmap.resize(sdf.width * sdf.height);
				std::memcpy(sdf.bitmap.data(), bitmap, sdf.bitmap.size());
				stbtt_FreeSDF(bitmap, nullptr);
				rect_lookup[hash] = rect_xywh(0, 0, sdf.width, sdf.height);

				Glyph& glyph = glyph_lookup[hash];
				glyph.x = float(sdf.xoff);
				glyph.y = float(sdf.yoff) + float(fontStyle.ascent) * fontScaling;
				glyph.width = float(sdf.width);
				glyph.height = float(sdf.height);


				// get bounding box for character (may be offset to account for chars that dip above or below the line
				int left, top, right, bottom;
				stbtt_GetCodepointBitmapBox(&fontStyle.fontInfo, code, fontScaling, fontScaling, &left, &top, &right, &bottom);
				glyph.off_x = float(left);
				glyph.off_y = float(top) + float(fontStyle.ascent) * fontScaling;
				glyph.off_w = float(right - left);
				glyph.off_h = float(bottom - top);
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
					const float height = (float)heightfromhash(hash);
					const FontStyle& fontStyle = fontStyles[style];
					rect_xywh& rect = it.second;
					Glyph& glyph = glyph_lookup[hash];
					SDF& sdf = sdf_lookup[hash];

					for (int row = 0; row < sdf.height; ++row)
					{
						uint8_t* dst = bitmap.data() + rect.x + (rect.y + row) * bitmapWidth;
						uint8_t* src = sdf.bitmap.data() + row * sdf.width;
						std::memcpy(dst, src, sdf.width);
					}

					//// Padding removal:
					//rect.x += sdf.padding;
					//rect.y += sdf.padding;
					//rect.w -= sdf.padding * 2;
					//rect.h -= sdf.padding * 2;

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
	void Draw_internal(const T* text, size_t text_length, const Params& params, CommandList cmd)
	{
		if (text_length <= 0)
		{
			return;
		}
		Cursor cursor = ParseText(text, text_length, params);

		Params newProps = params;
		if (params.h_align == WIFALIGN_CENTER)
			newProps.posX -= cursor.size.x / 2;
		else if (params.h_align == WIFALIGN_RIGHT)
			newProps.posX -= cursor.size.x;
		if (params.v_align == WIFALIGN_CENTER)
			newProps.posY -= cursor.size.y / 2;
		else if (params.v_align == WIFALIGN_BOTTOM)
			newProps.posY -= cursor.size.y;


		if (cursor.quadCount > 0)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(FontVertex) * cursor.quadCount * 4, cmd);
			if (!mem.IsValid())
			{
				return;
			}
			CommitText(mem.data);

			device->EventBegin("Font", cmd);

			device->BindPipelineState(&PSO, cmd);

			FontConstants font;
			FontPushConstants font_push;
			font_push.buffer_index = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
			font_push.buffer_offset = (uint32_t)mem.offset;
			font_push.texture_index = device->GetDescriptorIndex(&texture, SubresourceType::SRV);

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

				device->DrawInstanced(4, cursor.quadCount, 0, 0, cmd);
			}

			// font base render:
			XMStoreFloat4x4(&font.transform,
				XMMatrixTranslation((float)newProps.posX, (float)newProps.posY, 0)
				* Projection
			);
			device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

			font_push.color = newProps.color.rgba;
			device->PushConstants(&font_push, sizeof(font_push), cmd);

			device->DrawInstanced(4, cursor.quadCount, 0, 0, cmd);

			device->EventEnd(cmd);
		}
	}

	void SetCanvas(const wi::Canvas& current_canvas)
	{
		canvas = current_canvas;
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

	XMFLOAT2 TextSize(const char* text, const Params& params)
	{
		size_t text_length = strlen(text);
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).size;
	}
	XMFLOAT2 TextSize(const wchar_t* text, const Params& params)
	{
		size_t text_length = wcslen(text);
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).size;
	}
	XMFLOAT2 TextSize(const std::string& text, const Params& params)
	{
		if (text.empty())
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text.c_str(), text.length(), params).size;
	}
	XMFLOAT2 TextSize(const std::wstring& text, const Params& params)
	{
		if (text.empty())
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text.c_str(), text.length(), params).size;
	}

	float TextWidth(const char* text, const Params& params)
	{
		return TextSize(text, params).x;
	}
	float TextWidth(const wchar_t* text, const Params& params)
	{
		return TextSize(text, params).x;
	}
	float TextWidth(const std::string& text, const Params& params)
	{
		return TextSize(text, params).x;
	}
	float TextWidth(const std::wstring& text, const Params& params)
	{
		return TextSize(text, params).x;
	}

	float TextHeight(const char* text, const Params& params)
	{
		return TextSize(text, params).y;
	}
	float TextHeight(const wchar_t* text, const Params& params)
	{
		return TextSize(text, params).y;
	}
	float TextHeight(const std::string& text, const Params& params)
	{
		return TextSize(text, params).y;
	}
	float TextHeight(const std::wstring& text, const Params& params)
	{
		return TextSize(text, params).y;
	}

}
