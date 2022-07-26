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

namespace wi::font
{
#define WHITESPACE_SIZE ((float(params.size) + params.spacingX) * 0.25f)
#define TAB_SIZE (WHITESPACE_SIZE * 4)
#define LINEBREAK_SIZE ((float(params.size) + params.spacingY))

	namespace font_internal
	{
		static BlendState blendState;
		static RasterizerState rasterizerState;
		static DepthStencilState depthStencilState;
		static DepthStencilState depthStencilState_depth_test;

		static Shader vertexShader;
		static Shader pixelShader;
		static PipelineState PSO;
		static PipelineState PSO_depth_test;

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
		};
		static wi::unordered_map<int32_t, Glyph> glyph_lookup;
		static wi::unordered_map<int32_t, wi::rectpacker::Rect> rect_lookup;
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

		struct ParseStatus
		{
			Cursor cursor;
			uint32_t quadCount = 0;
			size_t last_word_begin = 0;
			bool start_new_word = false;
		};

		static thread_local wi::vector<FontVertex> vertexList;

		template<typename T>
		ParseStatus ParseText(const T* text, size_t text_length, Params params)
		{
			ParseStatus status;
			status.cursor = params.cursor;

			const FontStyle& fontStyle = fontStyles[params.style];
			const float fontScale = stbtt_ScaleForPixelHeight(&fontStyle.fontInfo, (float)params.size);
			vertexList.clear();

			auto word_wrap = [&] {
				status.start_new_word = true;
				if (status.last_word_begin > 0 && params.h_wrap >= 0 && status.cursor.position.x >= params.h_wrap - 1)
				{
					// Word ended and wrap detected, push down last word by one line:
					float word_offset = vertexList[status.last_word_begin].pos.x + WHITESPACE_SIZE;
					for (size_t i = status.last_word_begin; i < status.quadCount * 4; ++i)
					{
						vertexList[i].pos.x -= word_offset;
						vertexList[i].pos.y += LINEBREAK_SIZE;
					}
					status.cursor.position.x -= word_offset;
					status.cursor.position.y += LINEBREAK_SIZE;
					status.cursor.size.x = std::max(status.cursor.size.x, status.cursor.position.x);
					status.cursor.size.y = std::max(status.cursor.size.y, status.cursor.position.y + LINEBREAK_SIZE);
				}
			};

			status.cursor.size.y = status.cursor.position.y + LINEBREAK_SIZE;
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
					status.cursor.position.x = 0;
					status.cursor.position.y += LINEBREAK_SIZE;
				}
				else if (code == ' ')
				{
					word_wrap();
					status.cursor.position.x += WHITESPACE_SIZE;
				}
				else if (code == '\t')
				{
					word_wrap();
					status.cursor.position.x += TAB_SIZE;
				}
				else
				{
					const Glyph& glyph = glyph_lookup.at(hash);
					const float glyphWidth = glyph.width;
					const float glyphHeight = glyph.height;
					const float glyphOffsetX = glyph.x;
					const float glyphOffsetY = glyph.y;

					const size_t vertexID = size_t(status.quadCount) * 4;
					vertexList.resize(vertexID + 4);
					status.quadCount++;

					if (status.start_new_word)
					{
						status.last_word_begin = vertexID;
					}
					status.start_new_word = false;

					const float left = status.cursor.position.x + glyphOffsetX;
					const float right = left + glyphWidth;
					const float top = status.cursor.position.y + glyphOffsetY;
					const float bottom = top + glyphHeight;

					vertexList[vertexID + 0].pos = float2(left, top);
					vertexList[vertexID + 1].pos = float2(right, top);
					vertexList[vertexID + 2].pos = float2(left, bottom);
					vertexList[vertexID + 3].pos = float2(right, bottom);

					vertexList[vertexID + 0].uv = float2(glyph.tc_left, glyph.tc_top);
					vertexList[vertexID + 1].uv = float2(glyph.tc_right, glyph.tc_top);
					vertexList[vertexID + 2].uv = float2(glyph.tc_left, glyph.tc_bottom);
					vertexList[vertexID + 3].uv = float2(glyph.tc_right, glyph.tc_bottom);

					int advance, lsb;
					stbtt_GetCodepointHMetrics(&fontStyle.fontInfo, code, &advance, &lsb);
					status.cursor.position.x += advance * fontScale;

					status.cursor.position.x += params.spacingX;

					if (text_length > 1 && i < text_length - 1 && text[i + 1])
					{
						int code_next = (int)text[i + 1];
						int kern = stbtt_GetCodepointKernAdvance(&fontStyle.fontInfo, code, code_next);
						status.cursor.position.x += kern * fontScale;
					}
				}

				status.cursor.size.x = std::max(status.cursor.size.x, status.cursor.position.x);
				status.cursor.size.y = std::max(status.cursor.size.y, status.cursor.position.y + LINEBREAK_SIZE);
			}

			word_wrap();

			return status;
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

		desc.dss = &depthStencilState_depth_test;
		wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO_depth_test);
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
		rs.cull_mode = CullMode::NONE;
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
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendState = bd;

		DepthStencilState dsd;
		dsd.depth_enable = false;
		dsd.stencil_enable = false;
		depthStencilState = dsd;

		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::GREATER;
		depthStencilState_depth_test = dsd;

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

				wi::rectpacker::Rect rect = {};
				rect.w = sdf.width;
				rect.h = sdf.height;
				rect.id = hash;
				rect_lookup[hash] = rect;

				Glyph& glyph = glyph_lookup[hash];
				glyph.x = float(sdf.xoff);
				glyph.y = float(sdf.yoff) + float(fontStyle.ascent) * fontScaling;
				glyph.width = float(sdf.width);
				glyph.height = float(sdf.height);
			}
			pendingGlyphs.clear();

			// Setup packer, this will allocate memory if needed:
			static thread_local wi::rectpacker::State packer;
			packer.clear();
			for (auto& it : rect_lookup)
			{
				packer.add_rect(it.second);
			}

			// Perform packing and process the result if successful:
			if (packer.pack(4096))
			{
				// Retrieve texture atlas dimensions:
				const int bitmapWidth = packer.width;
				const int bitmapHeight = packer.height;
				const float inv_width = 1.0f / bitmapWidth;
				const float inv_height = 1.0f / bitmapHeight;

				// Create the CPU-side texture atlas and fill with transparency (0):
				wi::vector<uint8_t> bitmap(size_t(bitmapWidth) * size_t(bitmapHeight));
				std::fill(bitmap.begin(), bitmap.end(), 0);

				// Iterate all packed glyph rectangles:
				for (auto& rect : packer.rects)
				{
					const int32_t hash = rect.id;
					const wchar_t code = codefromhash(hash);
					const int style = stylefromhash(hash);
					const float height = (float)heightfromhash(hash);
					const FontStyle& fontStyle = fontStyles[style];
					Glyph& glyph = glyph_lookup[hash];
					SDF& sdf = sdf_lookup[hash];

					for (int row = 0; row < sdf.height; ++row)
					{
						uint8_t* dst = bitmap.data() + rect.x + (rect.y + row) * bitmapWidth;
						uint8_t* src = sdf.bitmap.data() + row * sdf.width;
						std::memcpy(dst, src, sdf.width);
					}

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
			else
			{
				assert(0); // rect packing failure
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
	Cursor Draw_internal(const T* text, size_t text_length, const Params& params, CommandList cmd)
	{
		if (text_length <= 0)
		{
			return Cursor();
		}
		ParseStatus status = ParseText(text, text_length, params);

		if (status.quadCount > 0)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(FontVertex) * status.quadCount * 4, cmd);
			if (!mem.IsValid())
			{
				return status.cursor;
			}
			CommitText(mem.data);

			FontConstants font = {};
			font.buffer_index = device->GetDescriptorIndex(&mem.buffer, SubresourceType::SRV);
			font.buffer_offset = (uint32_t)mem.offset;
			font.texture_index = device->GetDescriptorIndex(&texture, SubresourceType::SRV);
			if (font.buffer_index < 0 || font.texture_index < 0)
			{
				return status.cursor;
			}

			device->EventBegin("Font", cmd);

			if (params.isDepthTestEnabled())
			{
				device->BindPipelineState(&PSO_depth_test, cmd);
			}
			else
			{
				device->BindPipelineState(&PSO, cmd);
			}

			font.flags = 0;
			if (params.isHDR10OutputMappingEnabled())
			{
				font.flags |= FONT_FLAG_OUTPUT_COLOR_SPACE_HDR10_ST2084;
			}
			if (params.isLinearOutputMappingEnabled())
			{
				font.flags |= FONT_FLAG_OUTPUT_COLOR_SPACE_LINEAR;
				font.hdr_scaling = params.hdr_scaling;
			}

			XMFLOAT3 offset = XMFLOAT3(0, 0, 0);
			float vertical_flip = params.customProjection == nullptr ? 1.0f : -1.0f;
			if (params.h_align == WIFALIGN_CENTER)
				offset.x -= status.cursor.size.x / 2;
			else if (params.h_align == WIFALIGN_RIGHT)
				offset.x -= status.cursor.size.x;
			if (params.v_align == WIFALIGN_CENTER)
				offset.y -= status.cursor.size.y / 2 * vertical_flip;
			else if (params.v_align == WIFALIGN_BOTTOM)
				offset.y -= status.cursor.size.y * vertical_flip;

			XMMATRIX M = XMMatrixTranslation(offset.x, offset.y, offset.z);
			M = M * XMMatrixScaling(params.scaling, params.scaling, params.scaling);
			M = M * XMMatrixRotationZ(params.rotation);

			if (params.customRotation != nullptr)
			{
				M = M * (*params.customRotation);
			}

			M = M * XMMatrixTranslation(params.position.x, params.position.y, params.position.z);

			if (params.customProjection != nullptr)
			{
				M = XMMatrixScaling(1, -1, 1) * M; // reason: screen projection is Y down (like UV-space) and that is the common case for image rendering. But custom projections will use the "world space"
				M = M * (*params.customProjection);
			}
			else
			{
				// Asserts will check that a proper canvas was set for this cmd with wi::image::SetCanvas()
				//	The canvas must be set to have dpi aware rendering
				assert(canvas.width > 0);
				assert(canvas.height > 0);
				assert(canvas.dpi > 0);
				M = M * canvas.GetProjection();
			}

			if (params.shadowColor.getA() > 0)
			{
				// font shadow render:
				XMStoreFloat4x4(&font.transform, XMMatrixTranslation(params.shadow_offset_x, params.shadow_offset_y, 0) * M);
				font.color = params.shadowColor.rgba;
				font.sdf_threshold_top = wi::math::Lerp(float(SDF::onedge_value) / 255.0f, 0, std::max(0.0f, params.shadow_bolden));
				font.sdf_threshold_bottom = wi::math::Lerp(font.sdf_threshold_top, 0, std::max(0.0f, params.shadow_softness));
				device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

				device->DrawInstanced(4, status.quadCount, 0, 0, cmd);
			}

			// font base render:
			XMStoreFloat4x4(&font.transform, M);
			font.color = params.color.rgba;
			font.sdf_threshold_top = wi::math::Lerp(float(SDF::onedge_value) / 255.0f, 0, std::max(0.0f, params.bolden));
			font.sdf_threshold_bottom = wi::math::Lerp(font.sdf_threshold_top, 0, std::max(0.0f, params.softness));
			device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

			device->DrawInstanced(4, status.quadCount, 0, 0, cmd);

			device->EventEnd(cmd);
		}

		return status.cursor;
	}

	void SetCanvas(const wi::Canvas& current_canvas)
	{
		canvas = current_canvas;
	}

	Cursor Draw(const char* text, size_t text_length, const Params& params, CommandList cmd)
	{
		return Draw_internal(text, text_length, params, cmd);
	}
	Cursor Draw(const wchar_t* text, size_t text_length, const Params& params, CommandList cmd)
	{
		return Draw_internal(text, text_length, params, cmd);
	}
	Cursor Draw(const char* text, const Params& params, CommandList cmd)
	{
		return Draw_internal(text, strlen(text), params, cmd);
	}
	Cursor Draw(const wchar_t* text, const Params& params, CommandList cmd)
	{
		return Draw_internal(text, wcslen(text), params, cmd);
	}
	Cursor Draw(const std::string& text, const Params& params, CommandList cmd)
	{
		return Draw_internal(text.c_str(), text.length(), params, cmd);
	}
	Cursor Draw(const std::wstring& text, const Params& params, CommandList cmd)
	{
		return Draw_internal(text.c_str(), text.length(), params, cmd);
	}

	XMFLOAT2 TextSize(const char* text, size_t text_length, const Params& params)
	{
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).cursor.size;
	}
	XMFLOAT2 TextSize(const wchar_t* text, size_t text_length, const Params& params)
	{
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).cursor.size;
	}
	XMFLOAT2 TextSize(const char* text, const Params& params)
	{
		size_t text_length = strlen(text);
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).cursor.size;
	}
	XMFLOAT2 TextSize(const wchar_t* text, const Params& params)
	{
		size_t text_length = wcslen(text);
		if (text_length == 0)
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text, text_length, params).cursor.size;
	}
	XMFLOAT2 TextSize(const std::string& text, const Params& params)
	{
		if (text.empty())
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text.c_str(), text.length(), params).cursor.size;
	}
	XMFLOAT2 TextSize(const std::wstring& text, const Params& params)
	{
		if (text.empty())
		{
			return XMFLOAT2(0, 0);
		}
		return ParseText(text.c_str(), text.length(), params).cursor.size;
	}

	float TextWidth(const char* text, size_t text_length, const Params& params)
	{
		return TextSize(text, text_length, params).x;
	}
	float TextWidth(const wchar_t* text, size_t text_length, const Params& params)
	{
		return TextSize(text, text_length, params).x;
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

	float TextHeight(const char* text, size_t text_length, const Params& params)
	{
		return TextSize(text, text_length, params).y;
	}
	float TextHeight(const wchar_t* text, size_t text_length, const Params& params)
	{
		return TextSize(text, text_length, params).y;
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
