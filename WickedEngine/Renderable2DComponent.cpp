#include "Renderable2DComponent.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

Renderable2DComponent::Renderable2DComponent() : RenderableComponent()
{
	addLayer(DEFAULT_RENDERLAYER);
	GUI = wiGUI(GRAPHICSTHREAD_IMMEDIATE);
}


Renderable2DComponent::~Renderable2DComponent()
{
	Unload();
}

wiRenderTarget Renderable2DComponent::rtFinal;
void Renderable2DComponent::ResizeBuffers()
{
	wiRenderer::GetDevice()->WaitForGPU();

	FORMAT defaultTextureFormat = wiRenderer::GetDevice()->GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == (UINT)wiRenderer::GetDevice()->GetScreenWidth() &&
		lastBufferResHeight == (UINT)wiRenderer::GetDevice()->GetScreenHeight() &&
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = (UINT)wiRenderer::GetDevice()->GetScreenWidth();
		lastBufferResHeight = (UINT)wiRenderer::GetDevice()->GetScreenHeight();
		lastBufferFormat = defaultTextureFormat;
	}

	rtFinal.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), false, defaultTextureFormat);
}

void Renderable2DComponent::Initialize()
{
	ResizeBuffers();

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
		for (auto& y : x.items)
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
void Renderable2DComponent::Update(float dt)
{
	GetGUI().Update(dt);

	RenderableComponent::Update(dt);
}
void Renderable2DComponent::FixedUpdate()
{
	
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Update(getSpriteSpeed());
			}
		}
	}

	RenderableComponent::FixedUpdate();
}
void Renderable2DComponent::Render()
{
	rtFinal.Activate(GRAPHICSTHREAD_IMMEDIATE, 0.0f, 0.0f, 0.0f, 0.0f);

	wiRenderer::GetDevice()->EventBegin("Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Draw(GRAPHICSTHREAD_IMMEDIATE);
			}
			if (y.font != nullptr)
			{
				y.font->Draw(GRAPHICSTHREAD_IMMEDIATE);
			}
		}
	}
	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

	GetGUI().Render();

	RenderableComponent::Render();
}
void Renderable2DComponent::Compose()
{
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.presentFullScreen = true;
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;

	wiImage::Draw(rtFinal.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);

	RenderableComponent::Compose();
}


void Renderable2DComponent::addSprite(wiSprite* sprite, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			x.items.push_back(RenderItem2D());
			x.items.back().type = RenderItem2D::SPRITE;
			x.items.back().sprite = sprite;
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeSprite(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
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
		for (auto& y : x.items)
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
		for (auto& y : x.items)
		{
			if (y.sprite == sprite)
			{
				return y.order;
			}
		}
	}
	return 0;
}

void Renderable2DComponent::addFont(wiFont* font, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			x.items.push_back(RenderItem2D());
			x.items.back().type = RenderItem2D::FONT;
			x.items.back().font = font;
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeFont(wiFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
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
		for (auto& y : x.items)
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
		for (auto& y : x.items)
		{
			if (y.font == font)
			{
				return y.order;
			}
		}
	}
	return 0;
}


void Renderable2DComponent::addLayer(const std::string& name)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
			return;
	}
	RenderLayer2D layer = RenderLayer2D(name);
	layer.order = (int)layers.size();
	layers.push_back(layer);
	layers.back().items.clear();
}
void Renderable2DComponent::setLayerOrder(const std::string& name, int order)
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
		for (auto& y : x.items)
		{
			if (y.type == RenderItem2D::SPRITE && y.sprite == sprite)
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
		for (auto& y : x.items)
		{
			if (y.type == RenderItem2D::FONT && y.font == font)
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
				RenderLayer2D swap = layers[i];
				layers[i] = layers[j];
				layers[j] = swap;
			}
		}
	}
	for (auto& x : layers)
	{
		if (x.items.empty())
		{
			continue;
		}
		for (size_t i = 0; i < x.items.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < x.items.size(); ++j)
			{
				if (x.items[i].order > x.items[j].order)
				{
					RenderItem2D swap = x.items[i];
					x.items[i] = x.items[j];
					x.items[j] = swap;
				}
			}
		}
	}
}

void Renderable2DComponent::CleanLayers()
{
	for (auto& x : layers)
	{
		if (x.items.empty())
		{
			continue;
		}
		std::vector<RenderItem2D> itemsToRetain(0);
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr || y.font!=nullptr)
			{
				itemsToRetain.push_back(y);
			}
		}
		x.items.clear();
		x.items = itemsToRetain;
	}
}

