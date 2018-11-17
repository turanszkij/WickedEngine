#pragma once
#include "wiImage.h"
#include "wiGraphicsDevice.h"

class wiResourceManager;

class wiSprite
{
private:
	std::string texture, mask, normal;
	wiGraphicsTypes::Texture2D* texturePointer,*normalPointer,*maskPointer;
	wiResourceManager* ContentHolder;
public:
	wiSprite(wiResourceManager* contentHolder = nullptr);
	wiSprite(const std::string& newTexture, const std::string& newMask, const std::string& newNormal, wiResourceManager* contentHolder = nullptr);
	wiSprite(const std::string& newTexture, const std::string& newMask, wiResourceManager* contentHolder = nullptr);
	wiSprite(const std::string& newTexture, wiResourceManager* contentHolder = nullptr);
	void Init();
	void CreateReference(const std::string& newTexture, const std::string& newMask, const std::string& newNormal);
	void CleanUp();

	void Update(float);
	void Update();
	void Draw(wiGraphicsTypes::Texture2D* refracRes, GRAPHICSTHREAD threadID);
	void Draw(GRAPHICSTHREAD threadID);
	void DrawNormal(GRAPHICSTHREAD threadID);

	std::string name;

	wiImageParams effects;

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
	
	wiGraphicsTypes::Texture2D* getTexture(){return texturePointer;}
	void setTexture(wiGraphicsTypes::Texture2D* value){texturePointer=value;}
};

