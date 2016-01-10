#include "Renderable2DComponent.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

Renderable2DComponent::Renderable2DComponent()
{
	setSpriteSpeed(1.f);
	addLayer(DEFAULT_RENDERLAYER);

	RenderableComponent();
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

	RenderableComponent::Initialize();
}

void Renderable2DComponent::Load()
{
	RenderableComponent::Load();
}
void Renderable2DComponent::Unload()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->CleanUp();
				delete y.sprite;
			}
			if (y.font != nullptr)
			{
				y.font->CleanUp();
				delete y.font;
			}
		}
	}

	RenderableComponent::Unload();
}
void Renderable2DComponent::Start()
{
	RenderableComponent::Start();
}
void Renderable2DComponent::Update()
{
	
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Update(getSpriteSpeed());
			}
			if (y.font != nullptr)
			{
				// this is intentianally left blank
			}
		}
	}

	RenderableComponent::Update();
}
void Renderable2DComponent::Render()
{
	rtFinal.Activate(wiRenderer::getImmediateContext(), 0, 0, 0, 0);


	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Draw();
			}
			if (y.font != nullptr)
			{
				y.font->Draw();
			}
		}
	}

	RenderableComponent::Render();
}
void Renderable2DComponent::Compose()
{
	wiImage::Draw(rtFinal.shaderResource.back(), wiImageEffects((float)wiRenderer::GetScreenWidth(),(float)wiRenderer::GetScreenHeight()));

	RenderableComponent::Compose();
}


void Renderable2DComponent::addSprite(wiSprite* sprite, const string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			LayeredRenderEntity entity = LayeredRenderEntity();
			entity.type = LayeredRenderEntity::SPRITE;
			entity.sprite = sprite;
			x.entities.push_back(entity);
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeSprite(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite == sprite)
			{
				y.sprite = nullptr;
			}
		}
	}
	CleanLayers();
}
void Renderable2DComponent::clearSprites()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			y.sprite = nullptr;
		}
	}
	CleanLayers();
}
int Renderable2DComponent::getSpriteOrder(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite == sprite)
			{
				return y.order;
			}
		}
	}
	return 0;
}

void Renderable2DComponent::addFont(wiFont* font, const string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			LayeredRenderEntity entity = LayeredRenderEntity();
			entity.type = LayeredRenderEntity::FONT;
			entity.font = font;
			x.entities.push_back(entity);
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeFont(wiFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.font == font)
			{
				y.font = nullptr;
			}
		}
	}
	CleanLayers();
}
void Renderable2DComponent::clearFonts()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			y.font = nullptr;
		}
	}
	CleanLayers();
}
int Renderable2DComponent::getFontOrder(wiFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.font == font)
			{
				return y.order;
			}
		}
	}
	return 0;
}


void Renderable2DComponent::addLayer(const string& name)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
			return;
	}
	RenderLayer layer = RenderLayer(name);
	layer.order = layers.size();
	layers.push_back(layer);
	layers.back().entities.clear();
}
void Renderable2DComponent::setLayerOrder(const string& name, int order)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
		{
			x.order = order;
			break;
		}
	}
	SortLayers();
}
void Renderable2DComponent::SetSpriteOrder(wiSprite* sprite, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.type == LayeredRenderEntity::SPRITE && y.sprite == sprite)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void Renderable2DComponent::SetFontOrder(wiFont* font, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.type == LayeredRenderEntity::FONT && y.font == font)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void Renderable2DComponent::SortLayers()
{
	if (layers.empty())
	{
		return;
	}

	for (size_t i = 0; i < layers.size() - 1; ++i)
	{
		for (size_t j = i + 1; j < layers.size(); ++j)
		{
			if (layers[i].order > layers[j].order)
			{
				RenderLayer swap = layers[i];
				layers[i] = layers[j];
				layers[j] = swap;
			}
		}
	}
	for (auto& x : layers)
	{
		if (x.entities.empty())
		{
			continue;
		}
		for (size_t i = 0; i < x.entities.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < x.entities.size(); ++j)
			{
				if (x.entities[i].order > x.entities[j].order)
				{
					LayeredRenderEntity swap = x.entities[i];
					x.entities[i] = x.entities[j];
					x.entities[j] = swap;
				}
			}
		}
	}
}

void Renderable2DComponent::CleanLayers()
{
	for (auto& x : layers)
	{
		if (x.entities.empty())
		{
			continue;
		}
		vector<LayeredRenderEntity> cleanEntities(0);
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr || y.font!=nullptr)
			{
				cleanEntities.push_back(y);
			}
		}
		x.entities.clear();
		x.entities = cleanEntities;
	}
}

