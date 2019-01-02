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

	virtual void FixedUpdate(float speed);
	virtual void Update(float dt);
	void Draw(wiGraphicsTypes::Texture2D* refracRes, GRAPHICSTHREAD threadID);
	void Draw(GRAPHICSTHREAD threadID);
	void DrawNormal(GRAPHICSTHREAD threadID);

	std::string name;

	wiImageParams params;

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
		};
		struct WobbleAnim
		{
			XMFLOAT2 amount = XMFLOAT2(0, 0);	// how much the sprite wobbles in X and Y direction

			XMFLOAT2 corner_target_offsets[4] = {}; // internal use; you don't need to initialize
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
	
	wiGraphicsTypes::Texture2D* getTexture(){return texturePointer;}
	void setTexture(wiGraphicsTypes::Texture2D* value){texturePointer=value;}
};

