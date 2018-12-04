#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiColor.h"

namespace wiTextureHelper
{
	void Initialize();

	wiGraphicsTypes::Texture2D* getRandom64x64();
	wiGraphicsTypes::Texture2D* getColorGradeDefault();
	wiGraphicsTypes::Texture2D* getNormalMapDefault();
	wiGraphicsTypes::Texture2D* getBlackCubeMap();

	wiGraphicsTypes::Texture2D* getWhite();
	wiGraphicsTypes::Texture2D* getBlack();
	wiGraphicsTypes::Texture2D* getTransparent();
	wiGraphicsTypes::Texture2D* getColor(const wiColor& color);

	HRESULT CreateTexture(wiGraphicsTypes::Texture2D*& texture, const uint8_t* data, UINT width, UINT height, wiGraphicsTypes::FORMAT format = wiGraphicsTypes::FORMAT_R8G8B8A8_UNORM);
};

