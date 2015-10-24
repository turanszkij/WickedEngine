#pragma once
#include "RenderableComponent.h"

class wiSprite;
class wiFont;

class Renderable2DComponent :
	public RenderableComponent
{
private:
	float m_spriteSpeed;
	wiRenderTarget rtFinal;
protected:
	list<wiSprite*> m_sprites;
	list<wiFont*> m_fonts;
public:
	Renderable2DComponent();
	~Renderable2DComponent();

	virtual void Initialize();
	virtual void Load();
	virtual void Unload();
	virtual void Start();
	virtual void Update();
	virtual void Render();
	virtual void Compose();

	void addSprite(wiSprite* sprite);
	void setSpriteSpeed(float value){ m_spriteSpeed = value; }
	float getSpriteSpeed(){ return m_spriteSpeed; }
	void addFont(wiFont* font);
};

