#include "Renderable2DComponent.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

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
	rtFinal.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false);
}

void Renderable2DComponent::Load()
{

}
void Renderable2DComponent::Unload()
{
	for (wiSprite* x : m_sprites)
	{
		if (x != nullptr)
			x->CleanUp();
		SAFE_DELETE(x);
	}
	m_sprites.clear();

	for (wiFont* x : m_fonts)
	{
		if (x != nullptr)
			x->CleanUp();
		SAFE_DELETE(x);
	}
	m_fonts.clear();
}
void Renderable2DComponent::Start()
{

}
void Renderable2DComponent::Update()
{
	vector<wiSprite*> spritesToDelete(0);
	vector<wiFont*> fontsToDelete(0);

	for (wiSprite* x : m_sprites)
	{
		if (x != nullptr)
			x->Update(getSpriteSpeed());
		else
			spritesToDelete.push_back(x);
	}
	for (wiFont* x : m_fonts)
	{
		if (x == nullptr)
			fontsToDelete.push_back(x);
	}

	for (wiSprite* x : spritesToDelete)
	{
		m_sprites.remove(x);
	}
	for (wiFont* x : fontsToDelete)
	{
		m_fonts.remove(x);
	}
}
void Renderable2DComponent::Render()
{
	rtFinal.Activate(wiRenderer::getImmediateContext(), 0, 0, 0, 0);

	wiImage::BatchBegin();

	for (wiSprite* x : m_sprites)
	{
		if(x != nullptr)
			x->Draw();
	}

	for (wiFont* x : m_fonts)
	{
		if (x != nullptr)
			x->Draw();
	}
}
void Renderable2DComponent::Compose()
{
	wiImage::BatchBegin();
	wiImage::Draw(rtFinal.shaderResource.back(), wiImageEffects((float)wiRenderer::GetScreenWidth(),(float)wiRenderer::GetScreenHeight()));
}


void Renderable2DComponent::addSprite(wiSprite* sprite)
{
	m_sprites.push_back(sprite);
}
void Renderable2DComponent::addFont(wiFont* font)
{
	m_fonts.push_back(font);
}
