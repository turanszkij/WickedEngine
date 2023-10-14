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

#include "Utility/liberation_sans.h"
#include "Utility/stb_truetype.h"

#include <fstream>
#include <mutex>

using namespace wi::enums;
using namespace wi::graphics;

namespace wi::font
{
	namespace font_internal
	{
		enum DEPTH_TEST_MODE
		{
			DEPTH_TEST_OFF,
			DEPTH_TEST_ON,
			DEPTH_TEST_MODE_COUNT
		};
		static BlendState blendState;
		static RasterizerState rasterizerState;
		static DepthStencilState depthStencilStates[DEPTH_TEST_MODE_COUNT];

		static Shader vertexShader;
		static Shader pixelShader;
		static PipelineState PSO[DEPTH_TEST_MODE_COUNT];

		static thread_local wi::Canvas canvas;

		static Texture texture;

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
		static wi::vector<std::unique_ptr<FontStyle>> fontStyles;

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
			const FontStyle* fontStyle = nullptr;
		};
		static wi::unordered_map<int32_t, Glyph> glyph_lookup;
		static wi::unordered_map<int32_t, wi::rectpacker::Rect> rect_lookup;
		struct Bitmap
		{
			int width;
			int height;
			int xoff;
			int yoff;
			wi::vector<uint8_t> data;
		};
		static wi::unordered_map<int32_t, Bitmap> bitmap_lookup;
		union GlyphHash
		{
			struct
			{
				uint32_t code : 16;		// character code range supported: 0 - 65535
				uint32_t height : 10;	// height supported: 0 - 1023
				uint32_t style : 5;		// number of font styles supported: 0 - 31
				uint32_t sdf : 1;		// true or false
			} bits;
			uint32_t raw;
		};
		static_assert(sizeof(GlyphHash) == sizeof(uint32_t));
		static wi::unordered_set<uint32_t> pendingGlyphs;
		static std::mutex locker;

		struct ParseStatus
		{
			Cursor cursor;
			uint32_t quadCount = 0;
			size_t last_word_begin = 0;
			bool start_new_word = false;
		};

		static thread_local wi::vector<FontVertex> vertexList;
		ParseStatus ParseText(const wchar_t* text, size_t text_length, const Params& params)
		{
			ParseStatus status;
			status.cursor = params.cursor;

			vertexList.clear();

			const float whitespace_size = (float(params.size) + params.spacingX) * 0.25f;
			const float tab_size = whitespace_size * 4;
			const float linebreak_size = (float(params.size) + params.spacingY);

			auto word_wrap = [&] {
				status.start_new_word = true;
				if (status.last_word_begin > 0 && params.h_wrap >= 0 && status.cursor.position.x >= params.h_wrap - 1)
				{
					// Word ended and wrap detected, push down last word by one line:
					float word_offset = vertexList[status.last_word_begin].pos.x + whitespace_size;
					for (size_t i = status.last_word_begin; i < status.quadCount * 4; ++i)
					{
						vertexList[i].pos.x -= word_offset;
						vertexList[i].pos.y += linebreak_size;
					}
					status.cursor.position.x -= word_offset;
					status.cursor.position.y += linebreak_size;
					status.cursor.size.x = std::max(status.cursor.size.x, status.cursor.position.x);
					status.cursor.size.y = std::max(status.cursor.size.y, status.cursor.position.y + linebreak_size);
				}
			};

			status.cursor.size.y = status.cursor.position.y + linebreak_size;
			for (size_t i = 0; i < text_length; ++i)
			{
				int code = (int)text[i];
				GlyphHash hash;
				hash.bits.code = text[i];
				hash.bits.height = params.size;
				hash.bits.style = (uint32_t)params.style;
				hash.bits.sdf = params.isSDFRenderingEnabled() ? 1 : 0;

				if (glyph_lookup.count(hash.raw) == 0)
				{
					// glyph not packed yet, so add to pending list:
					std::scoped_lock lck(locker);
					pendingGlyphs.insert(hash.raw);
					continue;
				}

				if (code == '\n')
				{
					word_wrap();
					status.cursor.position.x = 0;
					status.cursor.position.y += linebreak_size;
				}
				else if (code == ' ')
				{
					word_wrap();
					status.cursor.position.x += whitespace_size;
				}
				else if (code == '\t')
				{
					word_wrap();
					status.cursor.position.x += tab_size;
				}
				else
				{
					const Glyph& glyph = glyph_lookup.at(hash.raw);
					const float glyphWidth = glyph.width;
					const float glyphHeight = glyph.height;
					const float glyphOffsetX = glyph.x;
					const float glyphOffsetY = glyph.y;
					const float fontScale = stbtt_ScaleForPixelHeight(&glyph.fontStyle->fontInfo, (float)params.size);

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
					stbtt_GetCodepointHMetrics(&glyph.fontStyle->fontInfo, code, &advance, &lsb);
					status.cursor.position.x += advance * fontScale;

					status.cursor.position.x += params.spacingX;

					if (text_length > 1 && i < text_length - 1 && text[i + 1])
					{
						int code_next = (int)text[i + 1];
						int kern = stbtt_GetCodepointKernAdvance(&glyph.fontStyle->fontInfo, code, code_next);
						status.cursor.position.x += kern * fontScale;
					}
				}

				status.cursor.size.x = std::max(status.cursor.size.x, status.cursor.position.x);
				status.cursor.size.y = std::max(status.cursor.size.y, status.cursor.position.y + linebreak_size);
			}

			word_wrap();

			return status;
		}

		thread_local static std::string char_temp_buffer;
		thread_local static std::wstring wchar_temp_buffer;
		ParseStatus ParseText(const char* text, size_t text_length, const Params& params)
		{
			// the temp buffers are used to avoid allocations of string objects:
			char_temp_buffer = text;
			wi::helper::StringConvert(char_temp_buffer, wchar_temp_buffer);
			return ParseText(wchar_temp_buffer.c_str(), wchar_temp_buffer.length(), params);
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

		for (int d = 0; d < DEPTH_TEST_MODE_COUNT; ++d)
		{
			PipelineStateDesc desc;
			desc.vs = &vertexShader;
			desc.ps = &pixelShader;
			desc.bs = &blendState;
			desc.dss = &depthStencilStates[d];
			desc.rs = &rasterizerState;
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;
			wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO[d]);
		}
	}
	void Initialize()
	{
		wi::Timer timer;

		// add default font if there is none yet:
		if (fontStyles.empty())
		{
			AddFontStyle("Liberation Sans", liberation_sans, sizeof(liberation_sans));
		}

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
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		blendState = bd;

		DepthStencilState dsd;
		dsd.depth_enable = false;
		dsd.stencil_enable = false;
		depthStencilStates[DEPTH_TEST_OFF] = dsd;

		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
		depthStencilStates[DEPTH_TEST_ON] = dsd;

		static wi::eventhandler::Handle handle1 = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wi::backlog::post("wi::font Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	void InvalidateAtlas()
	{
		texture = {};
		glyph_lookup.clear();
		rect_lookup.clear();
		bitmap_lookup.clear();
	}
	void UpdateAtlas(float upscaling)
	{
		std::scoped_lock lck(locker);

		upscaling = std::max(1.5f, upscaling); // add some minimum upscaling, especially for SDF
		static float upscaling_prev = 1;
		const float upscaling_rcp = 1.0f / upscaling;

		if (upscaling_prev != upscaling)
		{
			// If upscaling changed (DPI change), clear glyph caches, they will need to be re-rendered:
			InvalidateAtlas();
			upscaling_prev = upscaling;
		}

		// If there are pending glyphs, render them and repack the atlas:
		if (!pendingGlyphs.empty())
		{
			for (int32_t raw : pendingGlyphs)
			{
				GlyphHash hash;
				hash.raw = raw;
				const int code = (int)hash.bits.code;
				const float height = (float)hash.bits.height;
				const bool is_sdf = hash.bits.sdf ? true : false;
				uint32_t style = hash.bits.style;
				FontStyle* fontStyle = fontStyles[style].get();
				int glyphIndex = stbtt_FindGlyphIndex(&fontStyle->fontInfo, code);
				if (glyphIndex == 0)
				{
					// Try fallback to an other font style that has this character:
					style = 0;
					while (glyphIndex == 0 && style < fontStyles.size())
					{
						fontStyle = fontStyles[style].get();
						glyphIndex = stbtt_FindGlyphIndex(&fontStyle->fontInfo, code);
						style++;
					}
				}

				float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle->fontInfo, height * upscaling);

				Bitmap& bitmap = bitmap_lookup[hash.raw];
				bitmap.width = 0;
				bitmap.height = 0;
				bitmap.xoff = 0;
				bitmap.yoff = 0;

				if (is_sdf)
				{
					unsigned char* data = stbtt_GetGlyphSDF(
						&fontStyle->fontInfo,
						fontScaling,
						glyphIndex,
						(int)SDF::padding,
						(unsigned char)SDF::onedge_value,
						SDF::pixel_dist_scale,
						&bitmap.width,
						&bitmap.height,
						&bitmap.xoff,
						&bitmap.yoff
					);
					bitmap.data.resize(bitmap.width * bitmap.height);
					std::memcpy(bitmap.data.data(), data, bitmap.data.size());
					stbtt_FreeSDF(data, nullptr);
				}
				else
				{
					unsigned char* data = stbtt_GetGlyphBitmap(
						&fontStyle->fontInfo,
						fontScaling,
						fontScaling,
						glyphIndex,
						&bitmap.width,
						&bitmap.height,
						&bitmap.xoff,
						&bitmap.yoff
					);
					bitmap.data.resize(bitmap.width * bitmap.height);
					std::memcpy(bitmap.data.data(), data, bitmap.data.size());
					stbtt_FreeBitmap(data, nullptr);
				}

				wi::rectpacker::Rect rect = {};
				rect.w = bitmap.width + 2;
				rect.h = bitmap.height + 2;
				rect.id = hash.raw;
				rect_lookup[hash.raw] = rect;

				Glyph& glyph = glyph_lookup[hash.raw];
				glyph.x = float(bitmap.xoff) * upscaling_rcp;
				glyph.y = (float(bitmap.yoff) + float(fontStyle->ascent) * fontScaling) * upscaling_rcp;
				glyph.width = float(bitmap.width) * upscaling_rcp;
				glyph.height = float(bitmap.height) * upscaling_rcp;
				glyph.fontStyle = fontStyle;
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
				const int atlasWidth = packer.width;
				const int atlasHeight = packer.height;
				const float inv_width = 1.0f / atlasWidth;
				const float inv_height = 1.0f / atlasHeight;

				// Create the CPU-side texture atlas and fill with transparency (0):
				wi::vector<uint8_t> atlas(size_t(atlasWidth) * size_t(atlasHeight));
				std::fill(atlas.begin(), atlas.end(), 0);

				// Iterate all packed glyph rectangles:
				for (auto& rect : packer.rects)
				{
					rect.x += 1;
					rect.y += 1;
					rect.w -= 2;
					rect.h -= 2;

					const int32_t hash = rect.id;
					//const wchar_t code = codefromhash(hash);
					//const int style = stylefromhash(hash);
					//const float height = (float)heightfromhash(hash);
					Glyph& glyph = glyph_lookup[hash];
					Bitmap& bitmap = bitmap_lookup[hash];

					for (int row = 0; row < bitmap.height; ++row)
					{
						uint8_t* dst = atlas.data() + rect.x + (rect.y + row) * atlasWidth;
						uint8_t* src = bitmap.data.data() + row * bitmap.width;
						std::memcpy(dst, src, bitmap.width);
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
				wi::texturehelper::CreateTexture(texture, atlas.data(), atlasWidth, atlasHeight, Format::R8_UNORM);
				GetDevice()->SetName(&texture, "wi::font::texture");
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
		std::scoped_lock lck(locker);
		for (size_t i = 0; i < fontStyles.size(); i++)
		{
			const FontStyle& fontStyle = *fontStyles[i];
			if (fontStyle.name.compare(fontName) == 0)
			{
				return int(i);
			}
		}
		fontStyles.push_back(std::make_unique<FontStyle>());
		fontStyles.back()->Create(fontName);
		InvalidateAtlas(); // invalidate atlas, in case there were missing glyphs, upon adding new font style they could become valid
		return int(fontStyles.size() - 1);
	}
	int AddFontStyle(const std::string& fontName, const uint8_t* data, size_t size, bool copyData)
	{
		std::scoped_lock lck(locker);
		for (size_t i = 0; i < fontStyles.size(); i++)
		{
			const FontStyle& fontStyle = *fontStyles[i];
			if (fontStyle.name.compare(fontName) == 0)
			{
				return int(i);
			}
		}
		fontStyles.push_back(std::make_unique<FontStyle>());
		if (copyData)
		{
			fontStyles.back()->fontBuffer.resize(size);
			std::memcpy(fontStyles.back()->fontBuffer.data(), data, size);
			data = fontStyles.back()->fontBuffer.data();
		}
		fontStyles.back()->Create(fontName, data, size);
		InvalidateAtlas(); // invalidate atlas, in case there were missing glyphs, upon adding new font style they could become valid
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

			device->BindPipelineState(&PSO[params.isDepthTestEnabled()], cmd);

			font.flags = 0;
			if (params.isSDFRenderingEnabled())
			{
				font.flags |= FONT_FLAG_SDF_RENDERING;
			}
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
				font.color = params.shadowColor;
				font.color.x *= params.shadow_intensity;
				font.color.y *= params.shadow_intensity;
				font.color.z *= params.shadow_intensity;
				font.bolden = params.shadow_bolden;
				font.softness = params.shadow_softness * 0.5f;
				device->BindDynamicConstantBuffer(font, CBSLOT_FONT, cmd);

				device->DrawInstanced(4, status.quadCount, 0, 0, cmd);
			}

			// font base render:
			XMStoreFloat4x4(&font.transform, M);
			font.color = params.color;
			font.color.x *= params.intensity;
			font.color.y *= params.intensity;
			font.color.z *= params.intensity;
			font.bolden = params.bolden;
			font.softness = params.softness * 0.5f;
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

	Cursor TextCursor(const char* text, size_t text_length, const Params& params)
	{
		if (text_length == 0)
		{
			return {};
		}
		return ParseText(text, text_length, params).cursor;
	}
	Cursor TextCursor(const wchar_t* text, size_t text_length, const Params& params)
	{
		if (text_length == 0)
		{
			return {};
		}
		return ParseText(text, text_length, params).cursor;
	}
	Cursor TextCursor(const char* text, const Params& params)
	{
		size_t text_length = strlen(text);
		if (text_length == 0)
		{
			return {};
		}
		return ParseText(text, text_length, params).cursor;
	}
	Cursor TextCursor(const wchar_t* text, const Params& params)
	{
		size_t text_length = wcslen(text);
		if (text_length == 0)
		{
			return {};
		}
		return ParseText(text, text_length, params).cursor;
	}
	Cursor TextCursor(const std::string& text, const Params& params)
	{
		if (text.empty())
		{
			return {};
		}
		return ParseText(text.c_str(), text.length(), params).cursor;
	}
	Cursor TextCursor(const std::wstring& text, const Params& params)
	{
		if (text.empty())
		{
			return {};
		}
		return ParseText(text.c_str(), text.length(), params).cursor;
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
