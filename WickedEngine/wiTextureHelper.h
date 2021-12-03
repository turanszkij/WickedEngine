#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wi::texturehelper
{
	void Initialize();

	const wi::graphics::Texture* getRandom64x64();
	const wi::graphics::Texture* getColorGradeDefault();
	const wi::graphics::Texture* getNormalMapDefault();
	const wi::graphics::Texture* getBlackCubeMap();
	const wi::graphics::Texture* getUINT4();
	const wi::graphics::Texture* getBlueNoise();

	const wi::graphics::Texture* getWhite();
	const wi::graphics::Texture* getBlack();
	const wi::graphics::Texture* getTransparent();
	const wi::graphics::Texture* getColor(wi::Color color);

	bool CreateTexture(wi::graphics::Texture& texture, const uint8_t* data, uint32_t width, uint32_t height, wi::graphics::Format format = wi::graphics::Format::R8G8B8A8_UNORM);
};

