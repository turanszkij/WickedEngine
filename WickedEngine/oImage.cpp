#include "oImage.h"




oImage::oImage()
{
	Init();
}
oImage::oImage(const string& newTexture, const string& newMask, const string& newNormal){
	Init();
	CreateReference(newTexture,newMask,newNormal);
}
oImage::oImage(const string& newTexture, const string& newMask){
	Init();
	CreateReference(newTexture,newMask,"");
}
oImage::oImage(const string& newTexture){
	Init();
	CreateReference(newTexture,"","");
}
void oImage::Init(){
	texture="";
	mask="";
	normal="";
	name="";
	texturePointer=maskPointer=normalPointer=nullptr;
	effects=ImageEffects();
	anim=Anim();
}
void oImage::CreateReference(const string& newTexture, const string& newMask, const string& newNormal){
	if(newTexture.length()) {
		texture = newTexture;
		texturePointer = (ID3D11ShaderResourceView*)ResourceManager::add(newTexture);
	}
	if(newMask.length()) {
		maskPointer=(ID3D11ShaderResourceView*)ResourceManager::add(newMask);
		effects.setMaskMap( maskPointer );
		mask = newMask;
	}
	if(newNormal.length()) {
		normalPointer=(ID3D11ShaderResourceView*)ResourceManager::add(newNormal);
		//effects.setNormalMap( normalPointer );
		normal = newNormal;
	}
}
void oImage::CleanUp(){
}

void oImage::Draw(ID3D11ShaderResourceView* refracRes, ID3D11DeviceContext* context){
	if(effects.opacity<1 && ((effects.blendFlag==BLENDMODE_ADDITIVE && effects.fade<1) || effects.blendFlag!=BLENDMODE_ADDITIVE) ){
		effects.setRefractionMap(refracRes);
		Image::Draw(texturePointer,effects,context);
	}
}
void oImage::Draw(){
	oImage::Draw(NULL,Renderer::immediateContext);
}
void oImage::DrawNormal(ID3D11DeviceContext* context){
	if(normalPointer && effects.opacity<1 && ((effects.blendFlag==BLENDMODE_ADDITIVE && effects.fade<1) || effects.blendFlag!=BLENDMODE_ADDITIVE)){
		//effects.setRefractionMap(refracRes);

		ImageEffects effectsMod(effects);
		effectsMod.blendFlag=BLENDMODE_ADDITIVE;
		effectsMod.extractNormalMap=true;
		Image::Draw(normalPointer,effectsMod,context);
	}
}

void oImage::Update(float gameSpeed){
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
void oImage::Update(){
	Update(1);
}
