#include "wiSprite.h"
#include "wiResourceManager.h"
#include "wiImage.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;


wiSprite::wiSprite(wiResourceManager* contentHolder) :ContentHolder(contentHolder)
{
	Init();
}
wiSprite::wiSprite(const std::string& newTexture, const std::string& newMask, const std::string& newNormal, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,newMask,newNormal);
}
wiSprite::wiSprite(const std::string& newTexture, const std::string& newMask, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,newMask,"");
}
wiSprite::wiSprite(const std::string& newTexture, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,"","");
}
void wiSprite::Init()
{
	if (ContentHolder == nullptr)
	{
		ContentHolder = &wiResourceManager::GetGlobal();
	}
	texture = "";
	mask = "";
	normal = "";
	name = "";
	texturePointer = maskPointer = normalPointer = nullptr;
}
void wiSprite::CreateReference(const std::string& newTexture, const std::string& newMask, const std::string& newNormal)
{
	if (newTexture.length())
	{
		texture = newTexture;
		texturePointer = (Texture2D*)ContentHolder->add(newTexture);
	}
	if (newMask.length())
	{
		maskPointer = (Texture2D*)ContentHolder->add(newMask);
		params.setMaskMap(maskPointer);
		mask = newMask;
	}
	if (newNormal.length())
	{
		normalPointer = (Texture2D*)ContentHolder->add(newNormal);
		normal = newNormal;
	}
}
void wiSprite::CleanUp()
{
	ContentHolder->del(texture);
	ContentHolder->del(normal);
	ContentHolder->del(mask);
}

void wiSprite::Draw(Texture2D* refracRes, GRAPHICSTHREAD threadID)
{
	if(params.opacity>0 && ((params.blendFlag==BLENDMODE_ADDITIVE && params.fade<1) || params.blendFlag!=BLENDMODE_ADDITIVE) )
	{
		params.setRefractionSource(refracRes);
		wiImage::Draw(texturePointer,params,threadID);
	}
}
void wiSprite::Draw(GRAPHICSTHREAD threadID)
{
	wiSprite::Draw(nullptr, threadID);
}
void wiSprite::DrawNormal(GRAPHICSTHREAD threadID)
{
	if(normalPointer && params.opacity>0 && ((params.blendFlag==BLENDMODE_ADDITIVE && params.fade<1) || params.blendFlag!=BLENDMODE_ADDITIVE))
	{
		wiImageParams effectsMod(params);
		effectsMod.blendFlag=BLENDMODE_ADDITIVE;
		effectsMod.enableExtractNormalMap();
		wiImage::Draw(normalPointer,effectsMod,threadID);
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

		if (anim.drawRectAnim._currentFrame < anim.drawRectAnim.frameCount - 1)
		{
			// Advance frame counter if the animation is not finished:
			anim.drawRectAnim._currentFrame++;

			// Step one frame horizontally:
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
			params.drawRect.x -= params.drawRect.z * (anim.drawRectAnim.horizontalFrameCount - 1);
			params.drawRect.y -= params.drawRect.w * (anim.drawRectAnim.frameCount / anim.drawRectAnim.horizontalFrameCount - 1);
			anim.drawRectAnim._currentFrame = 0;
		}

	}

}
