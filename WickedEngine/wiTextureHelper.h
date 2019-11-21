#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wiTextureHelper
{
	void Initialize();

	const wiGraphics::Texture* getRandom64x64();
	const wiGraphics::Texture* getColorGradeDefault();
	const wiGraphics::Texture* getNormalMapDefault();
	const wiGraphics::Texture* getBlackCubeMap();

	const wiGraphics::Texture* getWhite();
	const wiGraphics::Texture* getBlack();
	const wiGraphics::Texture* getTransparent();
	const wiGraphics::Texture* getColor(wiColor color);

	HRESULT CreateTexture(wiGraphics::Texture& texture, const uint8_t* data, UINT width, UINT height, wiGraphics::FORMAT format = wiGraphics::FORMAT_R8G8B8A8_UNORM);
};

