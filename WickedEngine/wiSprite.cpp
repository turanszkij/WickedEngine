#include "wiSprite.h"
#include "wiResourceManager.h"
#include "wiImage.h"
#include "wiRenderer.h"
#include "wiRandom.h"

using namespace wiGraphics;


wiSprite::wiSprite(const std::string& newTexture, const std::string& newMask)
{
	if (!newTexture.empty())
	{
		texture = newTexture;
		texturePointer = (Texture2D*)wiResourceManager::GetGlobal().add(newTexture);
	}
	if (!newMask.empty())
	{
		maskPointer = (Texture2D*)wiResourceManager::GetGlobal().add(newMask);
		params.setMaskMap(maskPointer);
		mask = newMask;
	}
}
void wiSprite::Destroy()
{
	wiResourceManager::GetGlobal().del(texture);
	wiResourceManager::GetGlobal().del(mask);
	texturePointer = nullptr;
	maskPointer = nullptr;
}

void wiSprite::Draw(CommandList cmd) const
{
	wiImage::Draw(texturePointer, params, cmd);
}
void wiSprite::DrawNormal(CommandList cmd) const
{
	if (params.opacity > 0 && ((params.blendFlag == BLENDMODE_ADDITIVE && params.fade < 1) || params.blendFlag != BLENDMODE_ADDITIVE))
	{
		wiImageParams effectsMod(params);
		effectsMod.blendFlag = BLENDMODE_ADDITIVE;
		effectsMod.enableExtractNormalMap();
		wiImage::Draw(texturePointer, effectsMod, cmd);
	}
}

void wiSprite::FixedUpdate(float speed)
{

}

void wiSprite::Update(float dt)
{
	params.pos.x += anim.vel.x*dt;
	params.pos.y += anim.vel.y*dt;
	params.pos.z += anim.vel.z*dt;
	params.rotation += anim.rot*dt;
	params.scale.x += anim.scaleX*dt;
	params.scale.y += anim.scaleY*dt;
	params.opacity += anim.opa*dt;
	if (params.opacity >= 1) {
		if (anim.repeatable) { params.opacity = 0.99f; anim.opa *= -1; }
		else				params.opacity = 1;
	}
	else if (params.opacity <= 0) {
		if (anim.repeatable) { params.opacity = 0.01f; anim.opa *= -1; }
		else				params.opacity = 0;
	}
	params.fade += anim.fad*dt;
	if (params.fade >= 1) {
		if (anim.repeatable) { params.fade = 0.99f; anim.fad *= -1; }
		else				params.fade = 1;
	}
	else if (params.fade <= 0) {
		if (anim.repeatable) { params.fade = 0.01f; anim.fad *= -1; }
		else				params.fade = 0;
	}

	params.texOffset.x += anim.movingTexAnim.speedX*dt;
	params.texOffset.y += anim.movingTexAnim.speedY*dt;

	// Draw rect anim:
	if (anim.drawRectAnim.frameCount > 0)
	{
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
		// offset a random corner of the sprite:
		int corner = wiRandom::getRandom(0, 4);
		anim.wobbleAnim.corner_target_offsets[corner].x = (wiRandom::getRandom(0, 1000) * 0.001f - 0.5f) * anim.wobbleAnim.amount.x;
		anim.wobbleAnim.corner_target_offsets[corner].y = (wiRandom::getRandom(0, 1000) * 0.001f - 0.5f) * anim.wobbleAnim.amount.y;

		// All corners of the sprite will always follow the offset corners to obtain the wobble effect:
		wiImageParams default_params;
		for (int i = 0; i < 4; ++i)
		{
			XMVECTOR I = XMLoadFloat2(&default_params.corners[i]);
			XMVECTOR V = XMLoadFloat2(&params.corners[i]);
			XMVECTOR O = XMLoadFloat2(&anim.wobbleAnim.corner_target_offsets[i]);

			V = XMVectorLerp(V, I + O, 0.01f);

			XMStoreFloat2(&params.corners[i], V);
		}
	}

}
