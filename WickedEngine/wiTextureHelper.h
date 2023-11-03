#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wi::texturehelper
{
	void Initialize();

	const wi::graphics::Texture* getLogo();
	const wi::graphics::Texture* getRandom64x64();
	const wi::graphics::Texture* getColorGradeDefault();
	const wi::graphics::Texture* getNormalMapDefault();
	const wi::graphics::Texture* getBlackCubeMap();
	const wi::graphics::Texture* getUINT4();
	const wi::graphics::Texture* getBlueNoise();
	const wi::graphics::Texture* getWaterRipple();

	const wi::graphics::Texture* getWhite();
	const wi::graphics::Texture* getBlack();
	const wi::graphics::Texture* getTransparent();
	const wi::graphics::Texture* getColor(wi::Color color);

	bool CreateTexture(
		wi::graphics::Texture& texture,
		const uint8_t* data,
		uint32_t width,
		uint32_t height,
		wi::graphics::Format format = wi::graphics::Format::R8G8B8A8_UNORM,
		wi::graphics::Swizzle swizzle = {}
	);

	enum class GradientType
	{
		Linear,
		Circular,
		Angular,
	};
	enum class GradientFlags
	{
		None = 0,
		Inverse = 1 << 0,		// inverts resulting gradient
		Smoothstep = 1 << 1,	// applies smoothstep function to resulting gradient
		PerlinNoise = 1 << 2,	// applies perlin noise to gradient
		R16Unorm = 1 << 3,		// the texture will be created in R16_UNORM format instead of R8_UNORM
	};
	wi::graphics::Texture CreateGradientTexture(
		GradientType type,
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start = XMFLOAT2(0, 0),
		const XMFLOAT2& uv_end = XMFLOAT2(1, 0),
		GradientFlags gradient_flags = GradientFlags::None,
		wi::graphics::Swizzle swizzle = { wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R },
		float perlin_scale = 1,
		uint32_t perlin_seed = 1234u,
		int perlin_octaves = 8,
		float perlin_persistence = 0.5f
	);

	// Similar to CreateGradientTexture() with GradientType::Angular type but different parameters
	wi::graphics::Texture CreateCircularProgressGradientTexture(
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& direction = XMFLOAT2(0, 1),
		bool counter_clockwise = false,
		wi::graphics::Swizzle swizzle = { wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R, wi::graphics::ComponentSwizzle::R }
	);
};

template<>
struct enable_bitmask_operators<wi::texturehelper::GradientFlags> {
	static const bool enable = true;
};
