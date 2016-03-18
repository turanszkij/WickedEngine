#pragma once
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiGraphicsAPI.h"

class wiResourceManager;

class wiSprite : public wiImage
{
private:
	string texture, mask, normal;
	TextureView texturePointer,normalPointer,maskPointer;
	wiResourceManager* ContentHolder;
public:
	wiSprite(wiResourceManager* contentHolder = nullptr);
	wiSprite(const string& newTexture, const string& newMask, const string& newNormal, wiResourceManager* contentHolder = nullptr);
	wiSprite(const string& newTexture, const string& newMask, wiResourceManager* contentHolder = nullptr);
	wiSprite(const string& newTexture, wiResourceManager* contentHolder = nullptr);
	void Init();
	void CreateReference(const string& newTexture, const string& newMask, const string& newNormal);
	void CleanUp();

	void Update(float);
	void Update();
	void Draw(TextureView refracRes, GRAPHICSTHREAD threadID);
	void Draw();
	void DrawNormal(GRAPHICSTHREAD threadID);

	string name;

	wiImageEffects effects;

	struct Anim{
		struct MovingTexData{
			float speedX,speedY;
			MovingTexData(){
				speedX=speedY=0;
			}
		};
		struct DrawRecData{
			float onFrameChangeWait,elapsedFrames,currentFrame,frameCount;
			float jumpX,jumpY;
			float sizX,sizY;

			DrawRecData(){
				onFrameChangeWait=elapsedFrames=currentFrame=frameCount=0;
				jumpX=jumpY=0;
				sizX=sizY=0;
			}
		};

		bool repeatable;
		XMFLOAT3 vel;
		float rot,scaleX,scaleY,opa,fad;
		MovingTexData movingTexAnim;
		DrawRecData drawRecAnim;

		Anim(){
			repeatable=false;
			vel=XMFLOAT3(0,0,0);
			rot=scaleX=scaleY=fad=0;
			opa = 1;
			movingTexAnim=MovingTexData();
			drawRecAnim=DrawRecData();
		}
	};
	Anim anim;
	
	TextureView getTexture(){return texturePointer;}
	void setTexture(TextureView value){texturePointer=value;}
};

