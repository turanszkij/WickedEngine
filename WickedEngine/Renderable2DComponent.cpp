#include "Renderable2DComponent.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"

Renderable2DComponent::Renderable2DComponent()
{
	setSpriteSpeed(1.f);
	m_sprites.clear();
	m_fonts.clear();
}


Renderable2DComponent::~Renderable2DComponent()
{
	Unload();
}

void Renderable2DComponent::Initialize()
{
	
}

void Renderable2DComponent::Load()
{

}
void Renderable2DComponent::Unload()
{
	for (wiSprite* x : m_sprites)
	{
		x->CleanUp();
	}
	m_sprites.clear();
}
void Renderable2DComponent::Start()
{

}
void Renderable2DComponent::Update()
{
	for (wiSprite* x : m_sprites)
	{
		x->Update(getSpriteSpeed());
	}
}
void Renderable2DComponent::Compose()
{
	wiImage::BatchBegin();

	for (wiSprite* x : m_sprites)
	{
		x->Draw();
	}

	for (wiFont* x : m_fonts)
	{
		x->Draw();
	}
}


void Renderable2DComponent::addSprite(wiSprite* sprite)
{
	m_sprites.push_back(sprite);
}
void Renderable2DComponent::addFont(wiFont* font)
{
	m_fonts.push_back(font);
}
