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

void wiSprite::Update(float gameSpeed)
{
	params.pos.x += anim.vel.x*gameSpeed;
	params.pos.y += anim.vel.y*gameSpeed;
	params.pos.z += anim.vel.z*gameSpeed;
	params.rotation += anim.rot*gameSpeed;
	params.scale.x += anim.scaleX*gameSpeed;
	params.scale.y += anim.scaleY*gameSpeed;
	params.opacity += anim.opa*gameSpeed;
	if (params.opacity >= 1) {
		if (anim.repeatable) { params.opacity = 0.99f; anim.opa *= -1; }
		else				params.opacity = 1;
	}
	else if (params.opacity <= 0) {
		if (anim.repeatable) { params.opacity = 0.01f; anim.opa *= -1; }
		else				params.opacity = 0;
	}
	params.fade += anim.fad*gameSpeed;
	if (params.fade >= 1) {
		if (anim.repeatable) { params.fade = 0.99f; anim.fad *= -1; }
		else				params.fade = 1;
	}
	else if (params.fade <= 0) {
		if (anim.repeatable) { params.fade = 0.01f; anim.fad *= -1; }
		else				params.fade = 0;
	}

	params.texOffset.x += anim.movingTexAnim.speedX*gameSpeed;
	params.texOffset.y += anim.movingTexAnim.speedY*gameSpeed;

	if (anim.drawRecAnim.elapsedFrames >= anim.drawRecAnim.onFrameChangeWait) {
		params.drawRect.x += params.drawRect.z*anim.drawRecAnim.jumpX;
		params.drawRect.y += params.drawRect.w*anim.drawRecAnim.jumpY;

		if (anim.drawRecAnim.currentFrame >= anim.drawRecAnim.frameCount) {
			params.drawRect.z -= anim.drawRecAnim.sizX*anim.drawRecAnim.frameCount;
			params.drawRect.w -= anim.drawRecAnim.sizY*anim.drawRecAnim.frameCount;
			anim.drawRecAnim.currentFrame = 0;
		}
		else {
			params.drawRect.z += anim.drawRecAnim.sizX;
			params.drawRect.w += anim.drawRecAnim.sizY;
			anim.drawRecAnim.currentFrame++;
		}
		anim.drawRecAnim.elapsedFrames = 0;
	}
	else anim.drawRecAnim.elapsedFrames += gameSpeed;
}
void wiSprite::Update()
{
	Update(1);
}
