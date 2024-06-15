#include "wiTextureHelper.h"
#include "wiRandom.h"
#include "wiColor.h"
#include "wiBacklog.h"
#include "wiSpinLock.h"
#include "wiTimer.h"
#include "wiUnorderedMap.h"
#include "wiNoise.h"

// embedded image datas:
#include "logo.h"
#include "waterripple.h"

using namespace wi::graphics;

// from Utility/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp
extern float samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension);

namespace wi::texturehelper
{

	enum HELPERTEXTURES
	{
		HELPERTEXTURE_LOGO,
		HELPERTEXTURE_RANDOM64X64,
		HELPERTEXTURE_COLORGRADEDEFAULT,
		HELPERTEXTURE_BLACKCUBEMAP,
		HELPERTEXTURE_UINT4,
		HELPERTEXTURE_BLUENOISE,
		HELPERTEXTURE_WATERRIPPLE,
		HELPERTEXTURE_BLACK,
		HELPERTEXTURE_WHITE,
		HELPERTEXTURE_TRANSPARENT,
		HELPERTEXTURE_NORMALMAPDEFAULT,
		HELPERTEXTURE_COUNT
	};
	wi::graphics::Texture helperTextures[HELPERTEXTURE_COUNT];
	wi::unordered_map<unsigned long, wi::graphics::Texture> colorTextures;
	wi::SpinLock colorlock;

	void Initialize()
	{
		wi::Timer timer;

		GraphicsDevice* device = wi::graphics::GetDevice();

		// Logo
		{
			CreateTexture(helperTextures[HELPERTEXTURE_LOGO], wicked_engine_logo, 256, 256);
			device->SetName(&helperTextures[HELPERTEXTURE_LOGO], "HELPERTEXTURE_LOGO");
		}

		// Random64x64
		{
			uint8_t data[64 * 64 * 4];
			for (int i = 0; i < arraysize(data); i += 4)
			{
				data[i] = wi::random::GetRandom(0, 255);
				data[i + 1] = wi::random::GetRandom(0, 255);
				data[i + 2] = wi::random::GetRandom(0, 255);
				data[i + 3] = wi::random::GetRandom(0, 255);
			}

			CreateTexture(helperTextures[HELPERTEXTURE_RANDOM64X64], data, 64, 64);
			device->SetName(&helperTextures[HELPERTEXTURE_RANDOM64X64], "HELPERTEXTURE_RANDOM64X64");
		}

		// ColorGradeDefault
		{
			uint8_t data[256 * 16 * 4];
			for (uint8_t slice = 0; slice < 16; ++slice)
			{
				for (int x = 0; x < 16; ++x)
				{
					for (int y = 0; y < 16; ++y)
					{
						uint8_t r = x * 16 + x;
						uint8_t g = y * 16 + y;
						uint8_t b = slice * 16 + slice;

						int gridPos = (slice * 16 + y * 256 + x) * 4;
						data[gridPos] = r;
						data[gridPos + 1] = g;
						data[gridPos + 2] = b;
						data[gridPos + 3] = 255;
					}
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], data, 256, 16);
			device->SetName(&helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], "HELPERTEXTURE_COLORGRADEDEFAULT");
		}

		// BlackCubemap
		{
			const int width = 1;
			const int height = 1;

			TextureDesc texDesc;
			texDesc.width = width;
			texDesc.height = height;
			texDesc.mip_levels = 1;
			texDesc.array_size = 6;
			texDesc.format = Format::R8G8B8A8_UNORM;
			texDesc.sample_count = 1;
			texDesc.usage = Usage::DEFAULT;
			texDesc.bind_flags = BindFlag::SHADER_RESOURCE;
			texDesc.misc_flags = ResourceMiscFlag::TEXTURECUBE;

			SubresourceData pData[6];
			wi::Color d[6][width * height] = {}; // 6 images initialized to 0 (transparent black)

			for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
			{
				pData[cubeMapFaceIndex].data_ptr = &d[cubeMapFaceIndex][0];// description.data;
				pData[cubeMapFaceIndex].row_pitch = width * 4;
				pData[cubeMapFaceIndex].slice_pitch = 0;
			}

			device->CreateTexture(&texDesc, &pData[0], &helperTextures[HELPERTEXTURE_BLACKCUBEMAP]);
			device->SetName(&helperTextures[HELPERTEXTURE_BLACKCUBEMAP], "HELPERTEXTURE_BLACKCUBEMAP");
		}

		// UINT4:
		{
			uint8_t data[16] = {};
			CreateTexture(helperTextures[HELPERTEXTURE_UINT4], data, 1, 1, Format::R32G32B32A32_UINT);
			device->SetName(&helperTextures[HELPERTEXTURE_UINT4], "HELPERTEXTURE_UINT4");
		}

		// Blue Noise:
		{
			wi::Color bluenoise[128 * 128];

			for (int y = 0; y < 128; ++y)
			{
				for (int x = 0; x < 128; ++x)
				{
					const float f0 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 0);
					const float f1 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 1);
					const float f2 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 2);
					const float f3 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 3);

					bluenoise[x + y * 128] = wi::Color::fromFloat4(XMFLOAT4(f0, f1, f2, f3));
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_BLUENOISE], (const uint8_t*)bluenoise, 128, 128, Format::R8G8B8A8_UNORM);
			device->SetName(&helperTextures[HELPERTEXTURE_BLUENOISE], "HELPERTEXTURE_BLUENOISE");
		}

		// Water ripple:
		{
			TextureDesc desc;
			desc.width = 64;
			desc.height = 64;
			desc.mip_levels = 7;
			desc.format = Format::BC5_UNORM;
			desc.swizzle = { ComponentSwizzle::R,ComponentSwizzle::G,ComponentSwizzle::ONE,ComponentSwizzle::ONE };
			desc.bind_flags = BindFlag::SHADER_RESOURCE;

			const uint32_t data_stride = GetFormatStride(desc.format);
			const uint32_t block_size = GetFormatBlockSize(desc.format);
			const uint8_t* src = waterriple;
			SubresourceData initdata[7] = {};
			for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
			{
				const uint32_t num_blocks_x = std::max(1u, desc.width >> mip) / block_size;
				const uint32_t num_blocks_y = std::max(1u, desc.height >> mip) / block_size;
				initdata[mip].data_ptr = src;
				initdata[mip].row_pitch = num_blocks_x * data_stride;
				src += num_blocks_x * num_blocks_y * data_stride;
			}
			device->CreateTexture(&desc, initdata, &helperTextures[HELPERTEXTURE_WATERRIPPLE]);
			device->SetName(&helperTextures[HELPERTEXTURE_WATERRIPPLE], "HELPERTEXTURE_WATERRIPPLE");
		}

		// Single colors:
		{
			wi::Color color = wi::Color::Black();
			CreateTexture(helperTextures[HELPERTEXTURE_BLACK], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_BLACK], "HELPERTEXTURE_BLACK");

			color = wi::Color::White();
			CreateTexture(helperTextures[HELPERTEXTURE_WHITE], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_WHITE], "HELPERTEXTURE_WHITE");

			color = wi::Color::Transparent();
			CreateTexture(helperTextures[HELPERTEXTURE_TRANSPARENT], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_TRANSPARENT], "HELPERTEXTURE_TRANSPARENT");

			color = wi::Color(127, 127, 255, 255);
			CreateTexture(helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT], "HELPERTEXTURE_NORMALMAPDEFAULT");
		}

		wi::backlog::post("wi::texturehelper Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	const Texture* getLogo()
	{
		return &helperTextures[HELPERTEXTURE_LOGO];
	}
	const Texture* getRandom64x64()
	{
		return &helperTextures[HELPERTEXTURE_RANDOM64X64];
	}
	const Texture* getColorGradeDefault()
	{
		return &helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT];
	}
	const Texture* getNormalMapDefault()
	{
		return &helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT];
	}
	const Texture* getBlackCubeMap()
	{
		return &helperTextures[HELPERTEXTURE_BLACKCUBEMAP];
	}
	const Texture* getUINT4()
	{
		return &helperTextures[HELPERTEXTURE_UINT4];
	}
	const Texture* getBlueNoise()
	{
		return &helperTextures[HELPERTEXTURE_BLUENOISE];
	}
	const Texture* getWaterRipple()
	{
		return &helperTextures[HELPERTEXTURE_WATERRIPPLE];
	}
	const Texture* getWhite()
	{
		return &helperTextures[HELPERTEXTURE_WHITE];
	}
	const Texture* getBlack()
	{
		return &helperTextures[HELPERTEXTURE_BLACK];
	}
	const Texture* getTransparent()
	{
		return &helperTextures[HELPERTEXTURE_TRANSPARENT];
	}


	bool CreateTexture(
		Texture& texture,
		const void* data,
		uint32_t width,
		uint32_t height,
		Format format,
		Swizzle swizzle
	)
	{
		if (data == nullptr)
		{
			return false;
		}
		GraphicsDevice* device = wi::graphics::GetDevice();

		TextureDesc desc;
		desc.width = width;
		desc.height = height;
		desc.mip_levels = 1;
		desc.array_size = 1;
		desc.format = format;
		desc.sample_count = 1;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.swizzle = swizzle;

		SubresourceData InitData;
		InitData.data_ptr = data;
		InitData.row_pitch = width * GetFormatStride(format) / GetFormatBlockSize(format);

		return device->CreateTexture(&desc, &InitData, &texture);
	}

	Texture CreateGradientTexture(
		GradientType type,
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start,
		const XMFLOAT2& uv_end,
		GradientFlags flags,
		Swizzle swizzle,
		float perlin_scale,
		uint32_t perlin_seed,
		int perlin_octaves,
		float perlin_persistence
	)
	{
		wi::vector<uint8_t> data;
		wi::vector<uint16_t> data16;
		if (has_flag(flags, GradientFlags::R16Unorm))
		{
			data16.resize(width * height);
		}
		else
		{
			data.resize(width * height);
		}
		wi::noise::Perlin perlin;
		if (has_flag(flags, GradientFlags::PerlinNoise))
		{
			perlin.init(perlin_seed);
		}
		float aspect = float(height) / float(width);
		XMFLOAT2 perlin_scale2 = XMFLOAT2(perlin_scale, perlin_scale * aspect);

		switch (type)
		{
		default:
		case GradientType::Linear:
		{
			const XMVECTOR a = XMLoadFloat2(&uv_start);
			const XMVECTOR b = XMLoadFloat2(&uv_end);
			const float distance = XMVectorGetX(XMVector3Length(b - a));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const XMVECTOR point_on_line = wi::math::ClosestPointOnLineSegment(a, b, XMLoadFloat2(&uv));
					const float uv_distance = XMVectorGetX(XMVector3Length(point_on_line - a));
					float gradient = saturate(wi::math::InverseLerp(0, distance, uv_distance));
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = wi::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		case GradientType::Circular:
		{
			const XMVECTOR a = XMLoadFloat2(&uv_start);
			const XMVECTOR b = XMLoadFloat2(&uv_end);
			const float distance = XMVectorGetX(XMVector3Length(b - a));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const float uv_distance = wi::math::Clamp(XMVectorGetX(XMVector3Length(XMLoadFloat2(&uv) - a)), 0, distance);
					float gradient = saturate(wi::math::InverseLerp(0, distance, uv_distance));
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = wi::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		case GradientType::Angular:
		{
			XMFLOAT2 direction;
			XMStoreFloat2(&direction, XMVector2Normalize(XMLoadFloat2(&uv_end) - XMLoadFloat2(&uv_start)));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const XMFLOAT2 coord = XMFLOAT2(uv.x - uv_start.x, uv.y - uv_start.y);
					float gradient = wi::math::GetAngle(direction, coord) / XM_2PI;
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = wi::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		}

		Texture texture;
		if (has_flag(flags, GradientFlags::R16Unorm))
		{
			CreateTexture(texture, (const uint8_t*)data16.data(), width, height, Format::R16_UNORM, swizzle);
		}
		else
		{
			CreateTexture(texture, data.data(), width, height, Format::R8_UNORM, swizzle);
		}
		return texture;
	}

	Texture CreateCircularProgressGradientTexture(
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& direction,
		bool counter_clockwise,
		Swizzle swizzle
	)
	{
		wi::vector<uint8_t> data(width * height);
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				const XMFLOAT2 coord = XMFLOAT2((float(x) + 0.5f) / float(width) * 2 - 1, -((float(y) + 0.5f) / float(height) * 2 - 1));
				float gradient = wi::math::GetAngle(direction, coord) / XM_2PI;
				if (counter_clockwise)
				{
					gradient = 1 - gradient;
				}
				data[x + y * width] = uint8_t(gradient * 255);
			}
		}

		Texture texture;
		CreateTexture(texture, data.data(), width, height, Format::R8_UNORM, swizzle);
		return texture;
	}

	wi::graphics::Texture CreateLensDistortionNormalMap(
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start,
		float radius,
		float squish,
		float blend,
		float edge_smoothness
	)
	{
		XMFLOAT2 offset = XMFLOAT2(uv_start.x * 2 - 1, uv_start.y * 2 - 1);
		float scale = 1.0f / (radius * 2);
		float edge = 1.0f - edge_smoothness;

		wi::vector<uint32_t> data(width * height);
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				XMFLOAT2 uv = XMFLOAT2(float(x) / float(width - 1) * 2 - 1, float(y) / float(height - 1) * 2 - 1);

				uv.x -= offset.x;
				uv.y -= offset.y;
				uv.x *= scale;
				uv.y *= scale;

				if (width > height)
					uv.x *= float(width) / float(height);
				else
					uv.y *= float(height) / float(width);

				const float d = wi::math::Length(uv);
				const float dp = std::pow(saturate(d), squish);
				uv.x = uv.x * dp;
				uv.y = uv.y * dp;

				XMFLOAT2 color = XMFLOAT2(uv.x * 0.5f + 0.5f, uv.y * 0.5f + 0.5f);
				float s = smoothstep(1.0f, edge, d) * blend;
				color.x = lerp(0.5f, color.x, s);
				color.y = lerp(0.5f, color.y, s);
				data[x + y * width] = (uint32_t(color.x * 65535) & 0xFFFF) | (uint32_t(color.y * 65535) & 0xFFFF) << 16u;
			}
		}

		Texture texture;
		CreateTexture(texture, data.data(), width, height, Format::R16G16_UNORM);
		return texture;
	}

}
