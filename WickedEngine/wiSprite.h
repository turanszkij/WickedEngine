#pragma once
#include "wiImage.h"
#include "wiGraphicsDevice.h"
#include "wiResourceManager.h"
#include "wiRandom.h"
#include "wiArchive.h"
#include "wiECS.h"

#include <memory>
#include <string>

namespace wi
{
	class Sprite
	{
	public:
		enum FLAGS
		{
			EMPTY = 0,
			HIDDEN = 1 << 0,
			DISABLE_UPDATE = 1 << 1,
			CAMERA_FACING = 1 << 2,
			CAMERA_SCALING = 1 << 3,
		};
		uint32_t _flags = EMPTY;

		std::string textureName, maskName;

		Sprite(const std::string& newTexture = "", const std::string& newMask = "");
		virtual ~Sprite() = default;

		virtual void FixedUpdate();
		virtual void Update(float dt);
		virtual void Draw(wi::graphics::CommandList cmd) const;

		constexpr void SetHidden(bool value = true) { if (value) { _flags |= HIDDEN; } else { _flags &= ~HIDDEN; } }
		constexpr bool IsHidden() const { return _flags & HIDDEN; }
		constexpr void SetDisableUpdate(bool value = true) { if (value) { _flags |= DISABLE_UPDATE; } else { _flags &= ~DISABLE_UPDATE; } }
		constexpr void SetCameraFacing(bool value = true) { if (value) { _flags |= CAMERA_FACING; } else { _flags &= ~CAMERA_FACING; } }
		constexpr void SetCameraScaling(bool value = true) { if (value) { _flags |= CAMERA_SCALING; } else { _flags &= ~CAMERA_SCALING; } }

		constexpr bool IsDisableUpdate() const { return _flags & DISABLE_UPDATE; }
		constexpr bool IsCameraFacing() const { return _flags & CAMERA_FACING; }
		constexpr bool IsCameraScaling() const { return _flags & CAMERA_SCALING; }

		wi::image::Params params;
		wi::Resource textureResource;
		wi::Resource maskResource;

		const wi::graphics::Texture* GetTexture() const;

		struct Anim
		{
			struct MovingTexAnim
			{
				float speedX = 0; // the speed of texture scrolling animation in horizontal direction
				float speedY = 0; // the speed of texture scrolling animation in vertical direction
			};
			struct DrawRectAnim
			{
				float frameRate = 30; // target frame rate of the spritesheet animation (eg. 30, 60, etc.)
				int frameCount = 1; // how many frames are in the animation in total
				int horizontalFrameCount = 0; // how many horizontal frames there are (optional, use if the spritesheet contains multiple rows)

				float _elapsedTime = 0; // internal use; you don't need to initialize
				int _currentFrame = 0; // internal use; you don't need to initialize
				XMFLOAT4 originalDrawRect = XMFLOAT4(0, 0, 0, 0);  // internal use; you don't need to initialize

				void restart()
				{
					_elapsedTime = 0;
					_currentFrame = 0;
				}
			};
			struct WobbleAnim
			{
				XMFLOAT2 amount = XMFLOAT2(0, 0);	// how much the sprite wobbles in X and Y direction
				float speed = 1; // how fast the sprite wobbles

				float corner_angles[4]; // internal use; you don't need to initialize
				float corner_speeds[4]; // internal use; you don't need to initialize
				float corner_angles2[4]; // internal use; you don't need to initialize
				float corner_speeds2[4]; // internal use; you don't need to initialize
				WobbleAnim()
				{
					for (int i = 0; i < 4; ++i)
					{
						corner_angles[i] = wi::random::GetRandom(0, 1000) / 1000.0f * XM_2PI;
						corner_speeds[i] = wi::random::GetRandom(500, 1000) / 1000.0f;
						if (wi::random::GetRandom(0, 1) == 0)
						{
							corner_speeds[i] *= -1;
						}
						corner_angles2[i] = wi::random::GetRandom(0, 1000) / 1000.0f * XM_2PI;
						corner_speeds2[i] = wi::random::GetRandom(500, 1000) / 1000.0f;
						if (wi::random::GetRandom(0, 1) == 0)
						{
							corner_speeds2[i] *= -1;
						}
					}
				}
			};

			bool repeatable = false;
			XMFLOAT3 vel = XMFLOAT3(0, 0, 0);
			float rot = 0;
			float scaleX = 0;
			float scaleY = 0;
			float opa = 0;
			float fad = 0;
			MovingTexAnim movingTexAnim;
			DrawRectAnim drawRectAnim;
			WobbleAnim wobbleAnim;
		};
		Anim anim;

		const wi::graphics::Texture* getTexture() const
		{
			if (textureResource.IsValid())
			{
				return &textureResource.GetTexture();
			}
			return nullptr;
		}

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};
}
