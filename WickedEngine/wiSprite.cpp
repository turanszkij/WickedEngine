#include "wiSprite.h"
#include "wiImage.h"
#include "wiRandom.h"
#include "wiTextureHelper.h"
#include "wiHelper.h"

using namespace wi::graphics;

namespace wi
{

	Sprite::Sprite(const std::string& newTexture, const std::string& newMask)
	{
		if (!newTexture.empty())
		{
			textureName = newTexture;
			textureResource = wi::resourcemanager::Load(newTexture);
		}
		if (!newMask.empty())
		{
			maskName = newMask;
			maskResource = wi::resourcemanager::Load(newMask);
			params.setMaskMap(&maskResource.GetTexture());
		}
	}

	void Sprite::Draw(CommandList cmd) const
	{
		if (IsHidden())
			return;
		wi::image::Draw(GetTexture(), params, cmd);
	}

	void Sprite::FixedUpdate()
	{
		if (IsDisableUpdate())
			return;
	}

	void Sprite::Update(float dt)
	{
		if (IsDisableUpdate())
			return;
		params.pos.x += anim.vel.x * dt;
		params.pos.y += anim.vel.y * dt;
		params.pos.z += anim.vel.z * dt;
		params.rotation += anim.rot * dt;
		params.scale.x += anim.scaleX * dt;
		params.scale.y += anim.scaleY * dt;
		params.opacity += anim.opa * dt;
		if (params.opacity >= 1) {
			if (anim.repeatable) { params.opacity = 0.99f; anim.opa *= -1; }
			else				params.opacity = 1;
		}
		else if (params.opacity <= 0) {
			if (anim.repeatable) { params.opacity = 0.01f; anim.opa *= -1; }
			else				params.opacity = 0;
		}
		params.fade += anim.fad * dt;
		if (params.fade >= 1) {
			if (anim.repeatable) { params.fade = 0.99f; anim.fad *= -1; }
			else				params.fade = 1;
		}
		else if (params.fade <= 0) {
			if (anim.repeatable) { params.fade = 0.01f; anim.fad *= -1; }
			else				params.fade = 0;
		}

		params.texOffset.x += anim.movingTexAnim.speedX * dt;
		params.texOffset.y += anim.movingTexAnim.speedY * dt;

		// Draw rect anim:
		if (anim.drawRectAnim.frameCount > 1)
		{
			if (anim.drawRectAnim._currentFrame == 0)
			{
				anim.drawRectAnim.originalDrawRect = params.drawRect;
			}
			anim.drawRectAnim._elapsedTime += dt * anim.drawRectAnim.frameRate;
			if (anim.drawRectAnim._elapsedTime >= 1.0f)
			{
				// Reset timer:
				anim.drawRectAnim._elapsedTime = 0;

				if (anim.drawRectAnim.horizontalFrameCount == 0)
				{
					// If no horizontal frame count was specified, it means that all the frames are horizontal:
					anim.drawRectAnim.horizontalFrameCount = anim.drawRectAnim.frameCount;
				}

				// Advance frame counter:
				anim.drawRectAnim._currentFrame++;

				if (anim.drawRectAnim._currentFrame < anim.drawRectAnim.frameCount)
				{
					// Step one frame horizontally if animation is still playing:
					params.drawRect.x += params.drawRect.z;

					if (anim.drawRectAnim._currentFrame > 0 &&
						anim.drawRectAnim._currentFrame % anim.drawRectAnim.horizontalFrameCount == 0)
					{
						// if the animation is multiline, we reset horizontally and step downwards:
						params.drawRect.x -= params.drawRect.z * anim.drawRectAnim.horizontalFrameCount;
						params.drawRect.y += params.drawRect.w;
					}
				}
				else if (anim.repeatable)
				{
					// restart if repeatable:
					const int rewind_X = (anim.drawRectAnim.frameCount - 1) % anim.drawRectAnim.horizontalFrameCount;
					const int rewind_Y = (anim.drawRectAnim.frameCount - 1) / anim.drawRectAnim.horizontalFrameCount;
					params.drawRect.x -= params.drawRect.z * rewind_X;
					params.drawRect.y -= params.drawRect.w * rewind_Y;
					anim.drawRectAnim._currentFrame = 0;
				}

			}
		}

		// Wobble anim:
		if (anim.wobbleAnim.amount.x > 0 || anim.wobbleAnim.amount.y > 0)
		{
			// Rotate each corner on a scaled circle (ellipsoid):
			//	Since the rotations are randomized, it will look like a wobble effect
			//	Also use two circles on each other to achieve more random look
			wi::image::Params default_params; // contains corners in idle positions
			for (int i = 0; i < 4; ++i)
			{
				anim.wobbleAnim.corner_angles[i] += XM_2PI * anim.wobbleAnim.speed * anim.wobbleAnim.corner_speeds[i] * dt;
				anim.wobbleAnim.corner_angles2[i] += XM_2PI * anim.wobbleAnim.speed * anim.wobbleAnim.corner_speeds2[i] * dt;
				default_params.corners[i].x += std::sin(anim.wobbleAnim.corner_angles[i]) * anim.wobbleAnim.amount.x * 0.5f;
				default_params.corners[i].y += std::cos(anim.wobbleAnim.corner_angles[i]) * anim.wobbleAnim.amount.y * 0.5f;
				default_params.corners[i].x += std::sin(anim.wobbleAnim.corner_angles2[i]) * anim.wobbleAnim.amount.x * 0.25f;
				default_params.corners[i].y += std::cos(anim.wobbleAnim.corner_angles2[i]) * anim.wobbleAnim.amount.y * 0.25f;
				params.corners[i] = default_params.corners[i];
			}
		}

	}

	const wi::graphics::Texture* Sprite::GetTexture() const
	{
		return textureResource.IsValid() ? &textureResource.GetTexture() : wi::texturehelper::getWhite();
	}

	void Sprite::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> textureName;
			archive >> maskName;

			archive >> params._flags;
			archive >> params.pos;
			archive >> params.siz;
			archive >> params.scale;
			archive >> params.color;
			archive >> params.drawRect;
			archive >> params.drawRect2;
			archive >> params.texOffset;
			archive >> params.texOffset2;
			archive >> params.pivot;
			archive >> params.rotation;
			archive >> params.fade;
			archive >> params.opacity;
			archive >> params.intensity;
			archive >> params.hdr_scaling;
			archive >> params.mask_alpha_range_start;
			archive >> params.mask_alpha_range_end;
			archive >> params.border_soften;
			archive >> params.stencilRef;
			archive >> *(uint32_t*)&params.stencilComp;
			archive >> *(uint32_t*)&params.stencilRefMode;
			archive >> *(uint32_t*)&params.blendFlag;
			archive >> *(uint32_t*)&params.sampleFlag;
			archive >> *(uint32_t*)&params.quality;
			for (int i = 0; i < arraysize(params.corners); ++i)
			{
				archive >> params.corners[i];
				archive >> params.corners_rounding[i].radius;
				archive >> params.corners_rounding[i].segments;
			}

			archive >> anim.repeatable;
			archive >> anim.vel;
			archive >> anim.rot;
			archive >> anim.scaleX;
			archive >> anim.scaleY;
			archive >> anim.opa;
			archive >> anim.fad;
			archive >> anim.movingTexAnim.speedX;
			archive >> anim.movingTexAnim.speedY;
			archive >> anim.drawRectAnim.frameRate;
			archive >> anim.drawRectAnim.frameCount;
			archive >> anim.drawRectAnim.horizontalFrameCount;
			archive >> anim.wobbleAnim.amount;
			archive >> anim.wobbleAnim.speed;

			if (!textureName.empty() || !maskName.empty())
			{
				wi::jobsystem::Execute(seri.ctx, [&](wi::jobsystem::JobArgs args) {
					if (!textureName.empty())
					{
						textureName = dir + textureName;
						textureResource = wi::resourcemanager::Load(textureName);
					}
					if (!maskName.empty())
					{
						maskName = dir + maskName;
						maskResource = wi::resourcemanager::Load(maskName);
					}
				});
			}
		}
		else
		{
			seri.RegisterResource(textureName);
			seri.RegisterResource(maskName);

			archive << _flags;
			archive << wi::helper::GetPathRelative(dir, textureName);
			archive << wi::helper::GetPathRelative(dir, maskName);

			archive << params._flags;
			archive << params.pos;
			archive << params.siz;
			archive << params.scale;
			archive << params.color;
			if (anim.drawRectAnim.frameCount > 1)
			{
				params.drawRect = anim.drawRectAnim.originalDrawRect;
				anim.drawRectAnim.restart();
			}
			archive << params.drawRect;
			archive << params.drawRect2;
			archive << params.texOffset;
			archive << params.texOffset2;
			archive << params.pivot;
			archive << params.rotation;
			archive << params.fade;
			archive << params.opacity;
			archive << params.intensity;
			archive << params.hdr_scaling;
			archive << params.mask_alpha_range_start;
			archive << params.mask_alpha_range_end;
			archive << params.border_soften;
			archive << params.stencilRef;
			archive << (uint32_t)params.stencilComp;
			archive << (uint32_t)params.stencilRefMode;
			archive << (uint32_t)params.blendFlag;
			archive << (uint32_t)params.sampleFlag;
			archive << (uint32_t)params.quality;
			for (int i = 0; i < arraysize(params.corners); ++i)
			{
				archive << params.corners[i];
				archive << params.corners_rounding[i].radius;
				archive << params.corners_rounding[i].segments;
			}

			archive << anim.repeatable;
			archive << anim.vel;
			archive << anim.rot;
			archive << anim.scaleX;
			archive << anim.scaleY;
			archive << anim.opa;
			archive << anim.fad;
			archive << anim.movingTexAnim.speedX;
			archive << anim.movingTexAnim.speedY;
			archive << anim.drawRectAnim.frameRate;
			archive << anim.drawRectAnim.frameCount;
			archive << anim.drawRectAnim.horizontalFrameCount;
			archive << anim.wobbleAnim.amount;
			archive << anim.wobbleAnim.speed;
		}
	}

}
