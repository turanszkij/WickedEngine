#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wiTextureHelper
{
	void Initialize();

	const wiGraphicsTypes::Texture2D* getRandom64x64();
	const wiGraphicsTypes::Texture2D* getColorGradeDefault();
	const wiGraphicsTypes::Texture2D* getNormalMapDefault();
	const wiGraphicsTypes::Texture2D* getBlackCubeMap();

	const wiGraphicsTypes::Texture2D* getWhite();
	const wiGraphicsTypes::Texture2D* getBlack();
	const wiGraphicsTypes::Texture2D* getTransparent();
	const wiGraphicsTypes::Texture2D* getColor(const wiColor& color);

	HRESULT CreateTexture(wiGraphicsTypes::Texture2D& texture, const uint8_t* data, UINT width, UINT height, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM);
};

