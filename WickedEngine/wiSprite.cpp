#include "wiSprite.h"
#include "wiResourceManager.h"
#include "wiImageEffects.h"
#include "wiImage.h"
#include "wiRenderer.h"


wiSprite::wiSprite(wiResourceManager* contentHolder) :ContentHolder(contentHolder)
{
	Init();
}
wiSprite::wiSprite(const string& newTexture, const string& newMask, const string& newNormal, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,newMask,newNormal);
}
wiSprite::wiSprite(const string& newTexture, const string& newMask, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,newMask,"");
}
wiSprite::wiSprite(const string& newTexture, wiResourceManager* contentHolder) : ContentHolder(contentHolder)
{
	Init();
	CreateReference(newTexture,"","");
}
void wiSprite::Init(){
	if (ContentHolder == nullptr)
	{
		ContentHolder = wiResourceManager::GetGlobal();
	}
	texture="";
	mask="";
	normal="";
	name="";
	texturePointer=maskPointer=normalPointer=nullptr;
	effects=wiImageEffects();
	anim=Anim();
}
void wiSprite::CreateReference(const string& newTexture, const string& newMask, const string& newNormal){
	if(newTexture.length()) {
		texture = newTexture;
		texturePointer = (TextureView)ContentHolder->add(newTexture);
	}
	if(newMask.length()) {
		maskPointer = (TextureView)ContentHolder->add(newMask);
		effects.setMaskMap( maskPointer );
		mask = newMask;
	}
	if(newNormal.length()) {
		normalPointer = (TextureView)ContentHolder->add(newNormal);
		//effects.setNormalMap( normalPointer );
		normal = newNormal;
	}
}
void wiSprite::CleanUp(){
	ContentHolder->del(texture);
	ContentHolder->del(normal);
	ContentHolder->del(mask);
}

void wiSprite::Draw(TextureView refracRes, DeviceContext context){
	if(effects.opacity>0 && ((effects.blendFlag==BLENDMODE_ADDITIVE && effects.fade<1) || effects.blendFlag!=BLENDMODE_ADDITIVE) ){
		effects.setRefractionSource(refracRes);
		wiImage::Draw(texturePointer,effects,context);
	}
}
void wiSprite::Draw(){
	wiSprite::Draw(NULL,wiRenderer::getImmediateContext());
}
void wiSprite::DrawNormal(DeviceContext context){
	if(normalPointer && effects.opacity>0 && ((effects.blendFlag==BLENDMODE_ADDITIVE && effects.fade<1) || effects.blendFlag!=BLENDMODE_ADDITIVE)){
		//effects.setRefractionMap(refracRes);

		wiImageEffects effectsMod(effects);
		effectsMod.blendFlag=BLENDMODE_ADDITIVE;
		effectsMod.extractNormalMap=true;
		wiImage::Draw(normalPointer,effectsMod,context);
	}
}

void wiSprite::Update(float gameSpeed){
	effects.pos.x+=anim.vel.x*gameSpeed;
	effects.pos.y+=anim.vel.y*gameSpeed;
	effects.pos.z+=anim.vel.z*gameSpeed;
	effects.rotation+=anim.rot*gameSpeed;
	effects.scale.x+=anim.scaleX*gameSpeed;
	effects.scale.y+=anim.scaleY*gameSpeed;
	effects.opacity+=anim.opa*gameSpeed;
		if(effects.opacity>=1) {
			if(anim.repeatable) {effects.opacity=0.99f; anim.opa*=-1;}
			else				effects.opacity=1;
		}
		else if(effects.opacity<=0){
			if(anim.repeatable) {effects.opacity=0.01f; anim.opa*=-1;}
			else				effects.opacity=0;
		}
	effects.fade+=anim.fad*gameSpeed;
		if(effects.fade>=1) {
			if(anim.repeatable) {effects.fade=0.99f; anim.fad*=-1;}
			else				effects.fade=1;
		}
		else if(effects.fade<=0){
			if(anim.repeatable) {effects.fade=0.01f; anim.fad*=-1;}
			else				effects.fade=0;
		}
		
	effects.texOffset.x+=anim.movingTexAnim.speedX*gameSpeed;
	effects.texOffset.y+=anim.movingTexAnim.speedY*gameSpeed;
	
	if(anim.drawRecAnim.elapsedFrames>=anim.drawRecAnim.onFrameChangeWait){
		effects.drawRec.x+=effects.drawRec.z*anim.drawRecAnim.jumpX;
		effects.drawRec.y+=effects.drawRec.w*anim.drawRecAnim.jumpY;

		if(anim.drawRecAnim.currentFrame>=anim.drawRecAnim.frameCount){
			effects.drawRec.z-=anim.drawRecAnim.sizX*anim.drawRecAnim.frameCount;
			effects.drawRec.w-=anim.drawRecAnim.sizY*anim.drawRecAnim.frameCount;
			anim.drawRecAnim.currentFrame=0;
		}
		else{
			effects.drawRec.z+=anim.drawRecAnim.sizX;
			effects.drawRec.w+=anim.drawRecAnim.sizY;
			anim.drawRecAnim.currentFrame++;
		}
		anim.drawRecAnim.elapsedFrames=0;
	}
	else anim.drawRecAnim.elapsedFrames+=gameSpeed;
}
void wiSprite::Update(){
	Update(1);
}
