#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wiTextureHelper
{
	void Initialize();

	const wiGraphics::Texture2D* getRandom64x64();
	const wiGraphics::Texture2D* getColorGradeDefault();
	const wiGraphics::Texture2D* getNormalMapDefault();
	const wiGraphics::Texture2D* getBlackCubeMap();

	const wiGraphics::Texture2D* getWhite();
	const wiGraphics::Texture2D* getBlack();
	const wiGraphics::Texture2D* getTransparent();
	const wiGraphics::Texture2D* getColor(const wiColor& color);

	HRESULT CreateTexture(wiGraphics::Texture2D& texture, const uint8_t* data, UINT width, UINT height, wiGraphics::FORMAT format = wiGraphics::FORMAT_R8G8B8A8_UNORM);
};

