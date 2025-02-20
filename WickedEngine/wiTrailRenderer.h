#pragma once
#include "CommonInclude.h"
#include "wiVector.h"
#include "wiGraphicsDevice.h"
#include "wiEnums.h"
#include "wiScene_Decl.h"

namespace wi
{
	struct TrailRenderer
	{
		struct TrailPoint
		{
			XMFLOAT3 position = XMFLOAT3(0, 0, 0);
			float width = 1;
			XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		};
		wi::vector<TrailPoint> points;
		wi::vector<uint32_t> cuts;
		XMFLOAT4 color = XMFLOAT4(1, 1, 1, 1);
		wi::enums::BLENDMODE blendMode = wi::enums::BLENDMODE_ALPHA;
		uint32_t subdivision = 100;
		float width = 1;
		wi::graphics::Texture texture;
		wi::graphics::Texture texture2;
		XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
		XMFLOAT4 texMulAdd2 = XMFLOAT4(1, 1, 0, 0);

		// Adds a new point:
		void AddPoint(const XMFLOAT3& position, float width = 1, const XMFLOAT4& color = XMFLOAT4(1, 1, 1, 1));

		// Adds a new cut, which will start a new continuous curve from the next AddPoint():
		void Cut();

		// Fades the curve and remove points that can be removed:
		void Fade(float amount);

		void Draw(const wi::scene::CameraComponent& camera, wi::graphics::CommandList cmd) const;

		static void Initialize();
	};
}
